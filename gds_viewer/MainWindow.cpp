#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "GdsReader.hpp"
#include "GdsSceneView.hpp"

#include <QColorDialog>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTableWidgetItem>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

constexpr int kSceneBatchSize = 128;

QIcon makeColorSwatchIcon(const QColor& color) {
    QPixmap pixmap(24, 14);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Qt::black);
    painter.setBrush(color);
    painter.drawRect(0, 0, 23, 13);

    return QIcon(pixmap);
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui_(std::make_unique<Ui::MainWindow>()) {
    ui_->setupUi(this);
    setupUiState();

    transientStatusLabel_ = new QLabel(QStringLiteral("Откройте GDS файл"), this);
    transientStatusLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    statusBar()->addWidget(transientStatusLabel_);

    fileInfoLabel_ = new QLabel(QStringLiteral("Library: — | Structure: — | Elements: —"), this);
    fileInfoLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    statusBar()->addWidget(fileInfoLabel_, 1);

    setupStatusWidgets();

    selectedInfoLabel_ = new QLabel(QStringLiteral("Selected: —"), this);
    selectedInfoLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    statusBar()->addPermanentWidget(selectedInfoLabel_);

    connect(ui_->graphicsView, &GdsSceneView::selectedElementChanged,
            this, [this](const QString& text) {
                selectedInfoLabel_->setText(
                    text.isEmpty() ? QStringLiteral("Selected: —") : text);
            });

    connect(ui_->actionOpen, &QAction::triggered, this, &MainWindow::openFileDialog);
    connect(ui_->actionFit, &QAction::triggered, ui_->graphicsView, &GdsSceneView::fitAll);
    connect(ui_->actionExit, &QAction::triggered, this, &QWidget::close);

    connect(ui_->layerTableWidget, &QTableWidget::itemChanged, this, &MainWindow::handleLayerItemChanged);
    connect(ui_->layerTableWidget, &QTableWidget::cellClicked, this, &MainWindow::handleLayerCellClicked);
    connect(cancelButton_, &QPushButton::clicked, this, &MainWindow::requestCancelPipeline);

    setWindowTitle(QStringLiteral("GDS Viewer"));
}

MainWindow::~MainWindow() = default;

void MainWindow::loadFile(const QString& path) {
    if (path.isEmpty()) {
        return;
    }

    if (isPipelineRunning_) {
        transientStatusLabel_->setText(
            QStringLiteral("Загрузка уже выполняется. Дождитесь завершения или нажмите Cancel."));
        return;
    }

    runOpenPipeline(path);
}

DetachedTask MainWindow::runOpenPipeline(QString path) {
    const quint64 pipelineId = ++pipelineSerial_;
    cancelRequested_ = false;
    startPipelineUi(QStringLiteral("Подготовка загрузки: %1").arg(path));
    setProgressIndeterminate(QStringLiteral("Фоновый разбор GDS и подготовка батчей..."));

    PreparedSceneData prepared = co_await runInBackground(this, [path]() {
        return parseAndPrepare(path, kSceneBatchSize);
    });

    if (pipelineId != pipelineSerial_) {
        co_return;
    }

    if (!prepared.errorText.isEmpty()) {
        finishPipelineUi(QStringLiteral("Ошибка чтения GDS"));
        QMessageBox::critical(this, QStringLiteral("Ошибка чтения GDS"), prepared.errorText);
        co_return;
    }

    if (!prepared.library) {
        finishPipelineUi(QStringLiteral("Фоновая операция завершилась без результата"));
        QMessageBox::critical(this,
                              QStringLiteral("Ошибка чтения GDS"),
                              QStringLiteral("Фоновая операция завершилась без результата"));
        co_return;
    }

    if (cancelRequested_) {
        finishPipelineUi(QStringLiteral("Операция отменена"));
        co_return;
    }

    library_ = std::move(*prepared.library);
    currentFilePath_ = prepared.path;
    currentStructureIndex_ = prepared.structureIndex;

    if (currentStructureIndex_ < 0 || currentStructureIndex_ >= static_cast<int>(library_.structures.size())) {
        ui_->graphicsView->clearData();
        rebuildLayerTable();
        ui_->actionFit->setEnabled(false);
        setWindowTitle(QStringLiteral("GDS Viewer - %1").arg(path));

        fileInfoLabel_->setText(
            QStringLiteral("Library: %1 | Structure: — | Elements: 0")
                .arg(prepared.libraryName.isEmpty() ? QStringLiteral("—") : prepared.libraryName));
        finishPipelineUi(QStringLiteral("Файл прочитан, но структур нет"));
        co_return;
    }

    const auto& structure = library_.structures[static_cast<std::size_t>(currentStructureIndex_)];

    ui_->graphicsView->beginIncrementalBuild();
    ui_->layerTableWidget->setEnabled(false);
    ui_->actionFit->setEnabled(false);

    const int totalElements = std::max(1, prepared.totalElements);
    int builtElements = 0;
    setProgressValue(35, QStringLiteral("Построение сцены: %1").arg(prepared.structureName));

    for (const PreparedSceneBatchRange& batch : prepared.batches) {
        if (cancelRequested_) {
            ui_->graphicsView->clearData();
            rebuildLayerTable();
            ui_->actionFit->setEnabled(false);
            finishPipelineUi(QStringLiteral("Операция отменена"));
            co_return;
        }

        const int beginIndex = std::max(0, batch.beginIndex);
        const int endIndex = std::min(batch.endIndex, static_cast<int>(structure.elements.size()));
        for (int index = beginIndex; index < endIndex; ++index) {
            ui_->graphicsView->appendElement(structure.elements[static_cast<std::size_t>(index)]);
        }

        builtElements += std::max(0, endIndex - beginIndex);
        const double ratio = static_cast<double>(builtElements) / static_cast<double>(totalElements);
        const int progressValue = 35 + static_cast<int>(std::round(60.0 * ratio));
        setProgressValue(std::clamp(progressValue, 35, 95),
                         QStringLiteral("Построение сцены: %1 / %2")
                             .arg(builtElements)
                             .arg(prepared.totalElements));

        co_await nextGuiCycle(this);
    }

    if (cancelRequested_) {
        ui_->graphicsView->clearData();
        rebuildLayerTable();
        ui_->actionFit->setEnabled(false);
        finishPipelineUi(QStringLiteral("Операция отменена"));
        co_return;
    }

    ui_->graphicsView->finalizeIncrementalBuild(true);
    rebuildLayerTable();
    ui_->layerTableWidget->setEnabled(true);
    ui_->actionFit->setEnabled(ui_->graphicsView->hasContent());
    setWindowTitle(QStringLiteral("GDS Viewer - %1").arg(path));
    setProgressValue(100, QStringLiteral("Готово"));

    fileInfoLabel_->setText(
        QStringLiteral("Library: %1 | Structure: %2 | Elements: %3")
            .arg(prepared.libraryName)
            .arg(prepared.structureName)
            .arg(prepared.totalElements));

    finishPipelineUi(QStringLiteral("Готово"));
}

PreparedSceneData MainWindow::parseAndPrepare(const QString& path, int batchSize) {
    PreparedSceneData prepared;
    prepared.path = path;

    try {
        gds::GdsReader reader;
        auto library = std::make_shared<gds::GdsLibrary>(reader.readFile(path.toStdString()));

        prepared.library = library;
        prepared.libraryName = QString::fromStdString(library->name);

        if (library->structures.empty()) {
            return prepared;
        }

        prepared.structureIndex = 0;
        const auto& structure = library->structures.front();
        prepared.structureName = QString::fromStdString(structure.name);
        prepared.totalElements = static_cast<int>(structure.elements.size());

        const int safeBatchSize = std::max(1, batchSize);
        for (int begin = 0; begin < prepared.totalElements; begin += safeBatchSize) {
            prepared.batches.push_back(PreparedSceneBatchRange{
                begin,
                std::min(prepared.totalElements, begin + safeBatchSize)
            });
        }
    } catch (const std::exception& ex) {
        prepared.errorText = QString::fromLocal8Bit(ex.what());
    } catch (...) {
        prepared.errorText = QStringLiteral("Неизвестная ошибка при чтении GDS");
    }

    return prepared;
}

void MainWindow::requestCancelPipeline() {
    if (!isPipelineRunning_) {
        return;
    }

    cancelRequested_ = true;
    cancelButton_->setEnabled(false);
    transientStatusLabel_->setText(QStringLiteral("Отмена операции..."));
}

void MainWindow::startPipelineUi(const QString& statusText) {
    isPipelineRunning_ = true;
    ui_->actionOpen->setEnabled(false);
    ui_->actionFit->setEnabled(false);
    ui_->layerTableWidget->setEnabled(false);

    progressBar_->setVisible(true);
    progressBar_->setRange(0, 0);
    progressBar_->setValue(0);

    cancelButton_->setVisible(true);
    cancelButton_->setEnabled(true);

    transientStatusLabel_->setText(statusText);
}

void MainWindow::finishPipelineUi(const QString& statusText) {
    isPipelineRunning_ = false;
    cancelRequested_ = false;

    ui_->actionOpen->setEnabled(true);
    ui_->layerTableWidget->setEnabled(true);

    progressBar_->setVisible(false);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);

    cancelButton_->setVisible(false);
    cancelButton_->setEnabled(true);

    transientStatusLabel_->setText(statusText);
}

void MainWindow::setProgressIndeterminate(const QString& statusText) {
    progressBar_->setVisible(true);
    progressBar_->setRange(0, 0);
    transientStatusLabel_->setText(statusText);
}

void MainWindow::setProgressValue(int value, const QString& statusText) {
    if (progressBar_->minimum() == 0 && progressBar_->maximum() == 0) {
        progressBar_->setRange(0, 100);
    }

    progressBar_->setValue(std::clamp(value, 0, 100));
    transientStatusLabel_->setText(statusText);
}

void MainWindow::setupUiState() {
    ui_->layerTableWidget->setColumnCount(4);
    ui_->layerTableWidget->setHorizontalHeaderLabels(
        {QStringLiteral("On"), QStringLiteral("Color"), QStringLiteral("Layer"), QStringLiteral("Elements")});

    ui_->layerTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui_->layerTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui_->layerTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui_->layerTableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui_->layerTableWidget->setColumnWidth(1, 52);

    ui_->layerTableWidget->verticalHeader()->setVisible(false);
    ui_->layerTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui_->layerTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui_->layerTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui_->layerTableWidget->setAlternatingRowColors(true);

    ui_->graphicsView->setMinimumSize(QSize(600, 400));

    ui_->mainSplitter->setStretchFactor(0, 0);
    ui_->mainSplitter->setStretchFactor(1, 1);
    ui_->mainSplitter->setSizes({280, 920});
    ui_->mainSplitter->setOpaqueResize(false);

    ui_->actionFit->setEnabled(false);
}

void MainWindow::setupStatusWidgets() {
    progressBar_ = new QProgressBar(this);
    progressBar_->setFixedWidth(220);
    progressBar_->setTextVisible(true);
    progressBar_->setVisible(false);

    cancelButton_ = new QPushButton(QStringLiteral("Cancel"), this);
    cancelButton_->setVisible(false);

    statusBar()->addPermanentWidget(progressBar_);
    statusBar()->addPermanentWidget(cancelButton_);
}

void MainWindow::rebuildLayerTable() {
    const auto infos = ui_->graphicsView->layerInfos();

    ui_->layerTableWidget->blockSignals(true);
    ui_->layerTableWidget->clearContents();
    ui_->layerTableWidget->setRowCount(static_cast<int>(infos.size()));

    for (int row = 0; row < static_cast<int>(infos.size()); ++row) {
        const auto& info = infos[static_cast<std::size_t>(row)];

        auto* visibleItem = new QTableWidgetItem();
        visibleItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
        visibleItem->setCheckState(Qt::Checked);
        visibleItem->setData(Qt::UserRole, info.layer);

        auto* colorItem = new QTableWidgetItem();
        colorItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        colorItem->setData(Qt::UserRole, info.layer);
        colorItem->setData(Qt::UserRole + 1, info.color);
        colorItem->setText(QStringLiteral(" "));
        colorItem->setToolTip(QStringLiteral("Щёлкните, чтобы выбрать цвет слоя"));
        colorItem->setTextAlignment(Qt::AlignCenter);
        colorItem->setData(Qt::DecorationRole, makeColorSwatchIcon(info.color));

        auto* layerItem = new QTableWidgetItem(QString::number(info.layer));
        layerItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        layerItem->setData(Qt::UserRole, info.layer);

        auto* countItem = new QTableWidgetItem(QString::number(info.elementCount));
        countItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        countItem->setData(Qt::UserRole, info.layer);

        ui_->layerTableWidget->setItem(row, 0, visibleItem);
        ui_->layerTableWidget->setItem(row, 1, colorItem);
        ui_->layerTableWidget->setItem(row, 2, layerItem);
        ui_->layerTableWidget->setItem(row, 3, countItem);
    }

    ui_->layerTableWidget->blockSignals(false);
}

void MainWindow::openFileDialog() {
    const QString iniPath =
        QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");

    QSettings settings(iniPath, QSettings::IniFormat);

    QString startDir = settings.value("paths/lastOpenGdsDir").toString();

    if (startDir.isEmpty()) {
        if (!currentFilePath_.isEmpty()) {
            QFileInfo fi(currentFilePath_);
            startDir = fi.exists() ? fi.absolutePath() : currentFilePath_;
        }
    }

    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Open GDS"),
        startDir,
        QStringLiteral("GDS files (*.gds *.gds2 *.agf);;All files (*.*)"));

    if (!path.isEmpty()) {
        QFileInfo fi(path);
        settings.setValue("paths/lastOpenGdsDir", fi.absolutePath());

        currentFilePath_ = path;
        loadFile(path);
    }
}

void MainWindow::handleLayerItemChanged(QTableWidgetItem* item) {
    if (updatingLayerChecks_) {
        return;
    }

    if (item == nullptr || item->column() != 0) {
        return;
    }

    const Qt::CheckState newState = item->checkState();
    applyCheckStateToSelectedRows(item->row(), newState);
    applyLayerVisibilityFromTable();
}

void MainWindow::handleLayerCellClicked(int row, int column) {
    if (column != 1) {
        return;
    }

    QTableWidgetItem* colorItem = ui_->layerTableWidget->item(row, 1);
    if (colorItem == nullptr) {
        return;
    }

    QColor currentColor = colorItem->data(Qt::UserRole + 1).value<QColor>();
    if (!currentColor.isValid()) {
        const int layer = colorItem->data(Qt::UserRole).toInt();
        currentColor = ui_->graphicsView->layerColor(layer);
    }

    const QColor newColor = QColorDialog::getColor(
        currentColor,
        this,
        QStringLiteral("Выберите цвет слоя"),
        QColorDialog::DontUseNativeDialog);

    if (!newColor.isValid()) {
        return;
    }

    applyColorToSelectedRows(row, newColor);
}

void MainWindow::applyCheckStateToSelectedRows(int sourceRow, Qt::CheckState state) {
    auto* selectionModel = ui_->layerTableWidget->selectionModel();
    if (selectionModel == nullptr) {
        return;
    }

    const QModelIndexList selectedRows = selectionModel->selectedRows();
    if (selectedRows.size() <= 1) {
        return;
    }

    bool sourceRowIsSelected = false;
    for (const QModelIndex& index : selectedRows) {
        if (index.row() == sourceRow) {
            sourceRowIsSelected = true;
            break;
        }
    }

    if (!sourceRowIsSelected) {
        return;
    }

    updatingLayerChecks_ = true;
    {
        QSignalBlocker blocker(ui_->layerTableWidget);

        for (const QModelIndex& index : selectedRows) {
            const int row = index.row();
            if (row == sourceRow) {
                continue;
            }

            QTableWidgetItem* checkItem = ui_->layerTableWidget->item(row, 0);
            if (checkItem == nullptr) {
                continue;
            }

            checkItem->setCheckState(state);
        }
    }
    updatingLayerChecks_ = false;
}

void MainWindow::applyColorToSelectedRows(int sourceRow, const QColor& color) {
    auto* selectionModel = ui_->layerTableWidget->selectionModel();
    if (selectionModel == nullptr) {
        return;
    }

    const QModelIndexList selectedRows = selectionModel->selectedRows();

    bool sourceRowIsSelected = false;
    for (const QModelIndex& index : selectedRows) {
        if (index.row() == sourceRow) {
            sourceRowIsSelected = true;
            break;
        }
    }

    if (!sourceRowIsSelected || selectedRows.size() <= 1) {
        QTableWidgetItem* colorItem = ui_->layerTableWidget->item(sourceRow, 1);
        if (colorItem == nullptr) {
            return;
        }

        const int layer = colorItem->data(Qt::UserRole).toInt();
        ui_->graphicsView->setLayerColor(layer, color);
        updateLayerColorCell(sourceRow, color);
        return;
    }

    for (const QModelIndex& index : selectedRows) {
        const int row = index.row();

        QTableWidgetItem* colorItem = ui_->layerTableWidget->item(row, 1);
        if (colorItem == nullptr) {
            continue;
        }

        const int layer = colorItem->data(Qt::UserRole).toInt();
        ui_->graphicsView->setLayerColor(layer, color);
        updateLayerColorCell(row, color);
    }
}

void MainWindow::updateLayerColorCell(int row, const QColor& color) {
    QTableWidgetItem* colorItem = ui_->layerTableWidget->item(row, 1);
    if (colorItem == nullptr) {
        return;
    }

    colorItem->setData(Qt::UserRole + 1, color);
    colorItem->setData(Qt::DecorationRole, makeColorSwatchIcon(color));
}

void MainWindow::applyLayerVisibilityFromTable() {
    for (int row = 0; row < ui_->layerTableWidget->rowCount(); ++row) {
        QTableWidgetItem* checkItem = ui_->layerTableWidget->item(row, 0);
        if (checkItem == nullptr) {
            continue;
        }

        const int layer = checkItem->data(Qt::UserRole).toInt();
        const bool visible = (checkItem->checkState() == Qt::Checked);
        ui_->graphicsView->setLayerVisible(layer, visible);
    }
}
