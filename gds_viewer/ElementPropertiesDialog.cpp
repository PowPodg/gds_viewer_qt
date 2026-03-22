#include "ElementPropertiesDialog.hpp"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>

namespace {

QString formatPoint(const gds::Point& point) {
    return QStringLiteral("(%1, %2)").arg(point.x).arg(point.y);
}

struct Bounds {
    std::int32_t minX{};
    std::int32_t minY{};
    std::int32_t maxX{};
    std::int32_t maxY{};
};

std::optional<Bounds> computeBounds(const std::vector<gds::Point>& points) {
    if (points.empty()) {
        return std::nullopt;
    }

    Bounds b{points.front().x, points.front().y, points.front().x, points.front().y};
    for (const auto& p : points) {
        b.minX = std::min(b.minX, p.x);
        b.minY = std::min(b.minY, p.y);
        b.maxX = std::max(b.maxX, p.x);
        b.maxY = std::max(b.maxY, p.y);
    }
    return b;
}

QLineEdit* makeReadOnlyEdit(const QString& text, QWidget* parent) {
    auto* edit = new QLineEdit(text, parent);
    edit->setReadOnly(true);
    return edit;
}

} // namespace

ElementPropertiesDialog::ElementPropertiesDialog(const SceneElementInfo& info, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("%1 Properties").arg(info.elementType));
    resize(760, 460);

    auto* rootLayout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);
    rootLayout->addWidget(tabs);

    auto* geometryPage = new QWidget(this);
    auto* geometryLayout = new QFormLayout(geometryPage);
    geometryLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    geometryLayout->addRow(QStringLiteral("Type"), makeReadOnlyEdit(info.elementType, geometryPage));

    if (info.layer >= 0) {
        geometryLayout->addRow(QStringLiteral("Layer"), makeReadOnlyEdit(QString::number(info.layer), geometryPage));
    }

    if (!info.subtypeName.isEmpty() && info.subtypeValue.has_value()) {
        geometryLayout->addRow(info.subtypeName, makeReadOnlyEdit(QString::number(*info.subtypeValue), geometryPage));
    }

    if (!info.primaryText.isEmpty()) {
        geometryLayout->addRow(QStringLiteral("Value"), makeReadOnlyEdit(info.primaryText, geometryPage));
    }

    if (!info.points.empty()) {
        geometryLayout->addRow(QStringLiteral("Point count"),
                               makeReadOnlyEdit(QString::number(static_cast<int>(info.points.size())), geometryPage));
        geometryLayout->addRow(QStringLiteral("First point"), makeReadOnlyEdit(formatPoint(info.points.front()), geometryPage));
        geometryLayout->addRow(QStringLiteral("Last point"), makeReadOnlyEdit(formatPoint(info.points.back()), geometryPage));

        if (const auto bounds = computeBounds(info.points)) {
            geometryLayout->addRow(QStringLiteral("Lower left"),
                                   makeReadOnlyEdit(QStringLiteral("x=%1  y=%2").arg(bounds->minX).arg(bounds->minY), geometryPage));
            geometryLayout->addRow(QStringLiteral("Upper right"),
                                   makeReadOnlyEdit(QStringLiteral("x=%1  y=%2").arg(bounds->maxX).arg(bounds->maxY), geometryPage));

            const auto centerX = (static_cast<double>(bounds->minX) + static_cast<double>(bounds->maxX)) / 2.0;
            const auto centerY = (static_cast<double>(bounds->minY) + static_cast<double>(bounds->maxY)) / 2.0;
            geometryLayout->addRow(QStringLiteral("Center"),
                                   makeReadOnlyEdit(QStringLiteral("x=%1  y=%2").arg(centerX, 0, 'f', 3).arg(centerY, 0, 'f', 3), geometryPage));
            geometryLayout->addRow(QStringLiteral("Size"),
                                   makeReadOnlyEdit(QStringLiteral("w=%1  h=%2")
                                                        .arg(bounds->maxX - bounds->minX)
                                                        .arg(bounds->maxY - bounds->minY),
                                                    geometryPage));
        }
    }

    tabs->addTab(geometryPage, QStringLiteral("Geometry"));

    auto* userPropsPage = new QWidget(this);
    auto* userPropsLayout = new QVBoxLayout(userPropsPage);
    auto* propsTable = new QTableWidget(userPropsPage);
    propsTable->setColumnCount(2);
    propsTable->setHorizontalHeaderLabels({QStringLiteral("Key"), QStringLiteral("Value")});
    propsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    propsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    propsTable->verticalHeader()->setVisible(false);
    propsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    propsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    propsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    propsTable->setRowCount(static_cast<int>(info.properties.size()));

    for (int row = 0; row < static_cast<int>(info.properties.size()); ++row) {
        const auto& prop = info.properties[static_cast<std::size_t>(row)];
        auto* keyItem = new QTableWidgetItem(QStringLiteral("#%1").arg(prop.attr));
        auto* valueItem = new QTableWidgetItem(QString::fromStdString(prop.value));
        propsTable->setItem(row, 0, keyItem);
        propsTable->setItem(row, 1, valueItem);
    }

    userPropsLayout->addWidget(propsTable);
    tabs->addTab(userPropsPage, QStringLiteral("User Properties"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    rootLayout->addWidget(buttons);
}
