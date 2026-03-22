#pragma once

#include "CoroutinePipeline.hpp"
#include "GdsLibrary.hpp"
#include "PreparedSceneData.hpp"

#include <QColor>
#include <QMainWindow>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

#include <memory>

class QProgressBar;
class QPushButton;
class QTableWidgetItem;
class QLabel;

namespace Ui {
class MainWindow;
}

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void loadFile(const QString& path);

private:
    std::unique_ptr<Ui::MainWindow> ui_;

    gds::GdsLibrary library_;
    int currentStructureIndex_ = -1;
    QString currentFilePath_;
    bool updatingLayerChecks_ = false;
    bool isPipelineRunning_ = false;
    bool cancelRequested_ = false;
    quint64 pipelineSerial_ = 0;

    QProgressBar* progressBar_ = nullptr;
    QPushButton* cancelButton_ = nullptr;
    QLabel* transientStatusLabel_ = nullptr;
    QLabel* fileInfoLabel_ = nullptr;
    QLabel* selectedInfoLabel_ = nullptr;

    void setupUiState();
    void setupStatusWidgets();
    void rebuildLayerTable();
    void openFileDialog();
    void requestCancelPipeline();
    void startPipelineUi(const QString& statusText);
    void finishPipelineUi(const QString& statusText);
    void setProgressIndeterminate(const QString& statusText);
    void setProgressValue(int value, const QString& statusText);

    DetachedTask runOpenPipeline(QString path);
    static PreparedSceneData parseAndPrepare(const QString& path, int batchSize);

    void handleLayerItemChanged(QTableWidgetItem* item);
    void handleLayerCellClicked(int row, int column);

    void applyCheckStateToSelectedRows(int sourceRow, Qt::CheckState state);
    void applyColorToSelectedRows(int sourceRow, const QColor& color);
    void applyLayerVisibilityFromTable();
    void updateLayerColorCell(int row, const QColor& color);
};
