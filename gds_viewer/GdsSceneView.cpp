#include "GdsSceneView.hpp"

#include "HoverPathItem.hpp"
#include "HoverPolygonItem.hpp"

#include <QAction>
#include <QBrush>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsSimpleTextItem>
#include <QMenu>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <optional>
#include <set>
#include <type_traits>

namespace {

constexpr int kMetadataIndexRole = 0x4000;

QPointF toScenePoint(const gds::Point& p) {
    return QPointF(static_cast<qreal>(p.x), static_cast<qreal>(-p.y));
}

QPolygonF toPolygon(const std::vector<gds::Point>& points) {
    QPolygonF polygon;
    polygon.reserve(static_cast<int>(points.size()));
    for (const auto& point : points) {
        polygon << toScenePoint(point);
    }
    return polygon;
}

SceneElementInfo makeCommonInfo(const gds::ElementCommon& common) {
    SceneElementInfo info;
    info.properties = common.properties;
    return info;
}

void applySelectedStyleToGenericItem(QGraphicsItem* item, bool highlighted) {
    if (item == nullptr) {
        return;
    }

    if (auto* ellipse = qgraphicsitem_cast<QGraphicsEllipseItem*>(item)) {
        QPen pen = ellipse->pen();
        QBrush brush = ellipse->brush();

        if (highlighted) {
            pen.setColor(pen.color().lighter(250));
            pen.setWidthF(std::max<qreal>(2.0, pen.widthF() + 1.5));
            if (brush.style() != Qt::NoBrush) {
                QColor c = brush.color().lighter(240);
                c.setAlpha(std::min(255, c.alpha() + 120));
                brush.setColor(c);
            }
        }

        ellipse->setPen(pen);
        ellipse->setBrush(brush);
        return;
    }

    if (auto* text = qgraphicsitem_cast<QGraphicsSimpleTextItem*>(item)) {
        QColor color = text->brush().color();
        if (highlighted) {
            color = color.lighter(250);
        }
        text->setBrush(QBrush(color));
    }
}

} // namespace

QColor GdsSceneView::defaultColorForLayer(int layer) {
    return QColor::fromHsv((layer * 47) % 360, 180, 230);
}

GdsSceneView::GdsSceneView(QWidget* parent)
    : QGraphicsView(parent) {
    setScene(&scene_);
    setRenderHint(QPainter::Antialiasing, false);
    setOptimizationFlag(QGraphicsView::DontSavePainterState, true);
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    scene_.setItemIndexMethod(QGraphicsScene::BspTreeIndex);

    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
}

void GdsSceneView::clearData() {
    scene_.clear();
    layers_.clear();
    metadataStorage_.clear();
    metadataItems_.clear();
    selectedMetadataIndex_ = -1;
}

void GdsSceneView::beginIncrementalBuild() {
    clearData();
}

void GdsSceneView::appendElement(const gds::GdsElement& element) {
    std::visit(
        [this](const auto& value) {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, gds::Boundary>) {
                addBoundary(value);
            } else if constexpr (std::is_same_v<T, gds::Box>) {
                addBox(value);
            } else if constexpr (std::is_same_v<T, gds::Path>) {
                addPath(value);
            } else if constexpr (std::is_same_v<T, gds::Text>) {
                addTextMarker(value);
            } else if constexpr (std::is_same_v<T, gds::Node>) {
                addNode(value);
            } else if constexpr (std::is_same_v<T, gds::SRef>) {
                addSRef(value);
            } else if constexpr (std::is_same_v<T, gds::ARef>) {
                addARef(value);
            }
        },
        element);
}

void GdsSceneView::finalizeIncrementalBuild(bool fitView) {
    if (scene_.items().isEmpty()) {
        scene_.setSceneRect(QRectF());
        return;
    }

    const QRectF bounds = scene_.itemsBoundingRect().adjusted(-100.0, -100.0, 100.0, 100.0);
    scene_.setSceneRect(bounds);
    if (fitView) {
        fitInView(bounds, Qt::KeepAspectRatio);
    }
}

void GdsSceneView::loadStructure(const gds::GdsStructure& structure) {
    beginIncrementalBuild();
    for (const auto& element : structure.elements) {
        appendElement(element);
    }
    finalizeIncrementalBuild(true);
}

std::vector<GdsSceneView::LayerInfo> GdsSceneView::layerInfos() const {
    std::vector<LayerInfo> result;
    result.reserve(layers_.size());

    for (const auto& [layer, bucket] : layers_) {
        result.push_back(LayerInfo{
            layer,
            bucket.elementCount,
            bucket.color.isValid() ? bucket.color : defaultColorForLayer(layer)
        });
    }

    return result;
}

void GdsSceneView::setLayerVisible(int layer, bool visible) {
    const auto it = layers_.find(layer);
    if (it == layers_.end()) {
        return;
    }

    for (QGraphicsItem* item : it->second.items) {
        if (item != nullptr) {
            item->setVisible(visible);
        }
    }
}

void GdsSceneView::setLayerColor(int layer, const QColor& color) {
    if (!color.isValid()) {
        return;
    }

    const auto it = layers_.find(layer);
    if (it == layers_.end()) {
        return;
    }

    it->second.color = color;
    for (QGraphicsItem* item : it->second.items) {
        applyColorToItem(item, color);
    }

    if (selectedMetadataIndex_ >= 0) {
        setMetadataHighlighted(selectedMetadataIndex_, true);
    }

    scene_.update();
    viewport()->update();
}

QColor GdsSceneView::layerColor(int layer) const {
    const auto it = layers_.find(layer);
    if (it == layers_.end()) {
        return defaultColorForLayer(layer);
    }

    return it->second.color.isValid() ? it->second.color : defaultColorForLayer(layer);
}

bool GdsSceneView::hasContent() const {
    return !scene_.items().isEmpty();
}

void GdsSceneView::fitAll() {
    if (!scene_.items().isEmpty()) {
        fitInView(scene_.sceneRect(), Qt::KeepAspectRatio);
    }
}

void GdsSceneView::wheelEvent(QWheelEvent* event) {
    constexpr qreal zoomFactor = 1.15;
    if (event->angleDelta().y() > 0) {
        scale(zoomFactor, zoomFactor);
    } else {
        scale(1.0 / zoomFactor, 1.0 / zoomFactor);
    }
}

void GdsSceneView::mousePressEvent(QMouseEvent* event) {
    pressPos_ = event->pos();
    QGraphicsView::mousePressEvent(event);
}

void GdsSceneView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        const int dragDistance = (event->pos() - pressPos_).manhattanLength();
        if (dragDistance <= 3) {
            showContextMenuAt(event->pos());
            event->accept();
            return;
        }
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void GdsSceneView::addBoundary(const gds::Boundary& boundary) {
    if (boundary.xy.size() < 3) {
        return;
    }

    const QPolygonF polygon = toPolygon(boundary.xy);
    const QColor color = layerColor(static_cast<int>(boundary.layer));

    QPen pen(color.darker(130));
    pen.setCosmetic(true);
    pen.setWidthF(1.0);

    QBrush brush(QColor(color.red(), color.green(), color.blue(), 90), Qt::SolidPattern);

    auto* item = new HoverPolygonItem(polygon);
    item->setNormalStyle(pen, brush);
   // item->setToolTip(QStringLiteral("Layer %1").arg(boundary.layer));
    scene_.addItem(item);
    registerItem(static_cast<int>(boundary.layer), item, makeInfo(boundary));
}

void GdsSceneView::addBox(const gds::Box& box) {
    if (box.xy.size() < 3) {
        return;
    }

    const QPolygonF polygon = toPolygon(box.xy);
    const QColor color = layerColor(static_cast<int>(box.layer));

    QPen pen(color.darker(130));
    pen.setCosmetic(true);
    pen.setWidthF(1.0);

    QBrush brush(QColor(color.red(), color.green(), color.blue(), 90), Qt::SolidPattern);

    auto* item = new HoverPolygonItem(polygon);
    item->setNormalStyle(pen, brush);
    //item->setToolTip(QStringLiteral("Layer %1 BOX").arg(box.layer));
    scene_.addItem(item);
    registerItem(static_cast<int>(box.layer), item, makeInfo(box));
}

void GdsSceneView::addPath(const gds::Path& path) {
    if (path.xy.size() < 2) {
        return;
    }

    QPainterPath painterPath(toScenePoint(path.xy.front()));
    for (std::size_t i = 1; i < path.xy.size(); ++i) {
        painterPath.lineTo(toScenePoint(path.xy[i]));
    }

    const QColor color = layerColor(static_cast<int>(path.layer));
    QPen pen(color);
    pen.setCosmetic(true);
    pen.setWidthF(path.width.has_value() ? std::max<qreal>(1.0, static_cast<qreal>(*path.width)) : 1.0);

    auto* item = new HoverPathItem(painterPath);
    item->setNormalStyle(pen);
    //item->setToolTip(QStringLiteral("Layer %1 PATH").arg(path.layer));
    scene_.addItem(item);
    registerItem(static_cast<int>(path.layer), item, makeInfo(path));
}

void GdsSceneView::addTextMarker(const gds::Text& text) {
    const QPointF p = toScenePoint(text.xy);
    const QColor color = layerColor(static_cast<int>(text.layer));
    const auto info = makeInfo(text);

    auto* dot = scene_.addEllipse(p.x() - 4.0, p.y() - 4.0, 8.0, 8.0, QPen(color), QBrush(color));
    dot->setToolTip(QStringLiteral("Layer %1 TEXT").arg(text.layer));
    registerItem(static_cast<int>(text.layer), dot, info);

    auto* label = scene_.addSimpleText(QString::fromStdString(text.string));
    label->setBrush(QBrush(color));
    label->setPos(p + QPointF(6.0, -6.0));
    label->setData(kMetadataIndexRole, dot->data(kMetadataIndexRole));

    auto metaIt = metadataItems_.find(dot->data(kMetadataIndexRole).toInt());
    if (metaIt != metadataItems_.end()) {
        metaIt->second.push_back(label);
    }

    auto layerIt = layers_.find(static_cast<int>(text.layer));
    if (layerIt != layers_.end()) {
        layerIt->second.items.push_back(label);
    }
}

void GdsSceneView::addNode(const gds::Node& node) {
    if (node.xy.empty()) {
        return;
    }

    QPainterPath painterPath(toScenePoint(node.xy.front()));
    for (std::size_t i = 1; i < node.xy.size(); ++i) {
        painterPath.lineTo(toScenePoint(node.xy[i]));
    }

    const QColor color = layerColor(static_cast<int>(node.layer));
    QPen pen(color);
    pen.setCosmetic(true);
    pen.setWidthF(1.0);

    auto* item = new HoverPathItem(painterPath);
    item->setNormalStyle(pen);
    item->setToolTip(QStringLiteral("Layer %1 NODE").arg(node.layer));
    scene_.addItem(item);
    registerItem(static_cast<int>(node.layer), item, makeInfo(node));
}

void GdsSceneView::addSRef(const gds::SRef& sref) {
    const QPointF p = toScenePoint(sref.xy);
    auto* item = scene_.addEllipse(p.x() - 6.0, p.y() - 6.0, 12.0, 12.0, QPen(Qt::darkGray), QBrush(Qt::NoBrush));
    item->setToolTip(QStringLiteral("SREF: %1").arg(QString::fromStdString(sref.sname)));
    registerItem(-1, item, makeInfo(sref));

    auto* text = scene_.addSimpleText(QStringLiteral("SREF: %1").arg(QString::fromStdString(sref.sname)));
    text->setBrush(QBrush(Qt::darkGray));
    text->setPos(p + QPointF(8.0, -8.0));
    text->setToolTip(QStringLiteral("SREF: %1").arg(QString::fromStdString(sref.sname)));
    text->setData(kMetadataIndexRole, item->data(kMetadataIndexRole));

    metadataItems_[item->data(kMetadataIndexRole).toInt()].push_back(text);
}

void GdsSceneView::addARef(const gds::ARef& aref) {
    const QPointF p = toScenePoint(aref.origin);
    auto* item = scene_.addEllipse(p.x() - 6.0, p.y() - 6.0, 12.0, 12.0, QPen(Qt::darkGray), QBrush(Qt::NoBrush));
    item->setToolTip(QStringLiteral("AREF: %1").arg(QString::fromStdString(aref.sname)));
    registerItem(-1, item, makeInfo(aref));

    auto* text = scene_.addSimpleText(QStringLiteral("AREF: %1").arg(QString::fromStdString(aref.sname)));
    text->setBrush(QBrush(Qt::darkGray));
    text->setPos(p + QPointF(8.0, -8.0));
    text->setToolTip(QStringLiteral("AREF: %1").arg(QString::fromStdString(aref.sname)));
    text->setData(kMetadataIndexRole, item->data(kMetadataIndexRole));

    metadataItems_[item->data(kMetadataIndexRole).toInt()].push_back(text);
}

void GdsSceneView::registerItem(int layer, QGraphicsItem* item, const SceneElementInfo& info) {
    if (item == nullptr) {
        return;
    }

    if (layer >= 0) {
        auto& bucket = layers_[layer];
        if (!bucket.color.isValid()) {
            bucket.color = defaultColorForLayer(layer);
        }
        bucket.elementCount += 1;
        bucket.items.push_back(item);
    }

    const int metadataIndex = storeMetadata(info);
    item->setData(kMetadataIndexRole, metadataIndex);
    metadataItems_[metadataIndex].push_back(item);
}

int GdsSceneView::storeMetadata(const SceneElementInfo& info) {
    metadataStorage_.push_back(std::make_unique<SceneElementInfo>(info));
    return static_cast<int>(metadataStorage_.size() - 1);
}

void GdsSceneView::showPropertiesForMetadataIndex(int index) {
    if (index < 0 || index >= static_cast<int>(metadataStorage_.size())) {
        return;
    }

    ElementPropertiesDialog dialog(*metadataStorage_[static_cast<std::size_t>(index)], this);
    dialog.exec();
}

std::vector<GdsSceneView::CandidateItem> GdsSceneView::collectCandidatesAt(const QPoint& viewPos) const {
    const QPointF scenePos = mapToScene(viewPos);
    const QList<QGraphicsItem*> itemsAtPoint = scene_.items(scenePos, Qt::IntersectsItemShape, Qt::DescendingOrder, transform());

    std::vector<CandidateItem> candidates;
    std::set<int> seen;

    for (QGraphicsItem* item : itemsAtPoint) {
        if (item == nullptr || !item->isVisible()) {
            continue;
        }

        bool ok = false;
        const int index = item->data(kMetadataIndexRole).toInt(&ok);
        if (!ok || index < 0 || index >= static_cast<int>(metadataStorage_.size())) {
            continue;
        }

        if (!seen.insert(index).second) {
            continue;
        }

        const auto& info = *metadataStorage_[static_cast<std::size_t>(index)];
        const QRectF rect = item->sceneBoundingRect();
        candidates.push_back(CandidateItem{index, makeCandidateTitle(info), rect.width() * rect.height()});
    }

    std::sort(candidates.begin(), candidates.end(), [](const CandidateItem& a, const CandidateItem& b) {
        if (std::abs(a.area - b.area) > 1e-9) {
            return a.area < b.area;
        }
        return a.title < b.title;
    });

    return candidates;
}

QString GdsSceneView::makeCandidateTitle(const SceneElementInfo& info) const {
    QString title;
    if (info.layer >= 0) {
        title = QStringLiteral("Layer %1 | %2").arg(info.layer).arg(info.elementType);
    } else {
        title = info.elementType;
    }

    if (!info.subtypeName.isEmpty() && info.subtypeValue.has_value()) {
        title += QStringLiteral(" | %1=%2").arg(info.subtypeName).arg(*info.subtypeValue);
    }

    if (!info.primaryText.isEmpty()) {
        title += QStringLiteral(" | %1").arg(info.primaryText);
    }

    return title;
}

QString GdsSceneView::makeSelectedText(const SceneElementInfo& info) const {
    QString text = QStringLiteral("Selected: Layer %1 | %2")
        .arg(info.layer)
        .arg(info.elementType);

    if (!info.subtypeName.isEmpty() && info.subtypeValue.has_value()) {
        text += QStringLiteral(" | %1=%2")
            .arg(info.subtypeName)
            .arg(*info.subtypeValue);
    }

    if (!info.primaryText.isEmpty()) {
        text += QStringLiteral(" | %1").arg(info.primaryText);
    }

    return text;
}

void GdsSceneView::showContextMenuAt(const QPoint& viewPos) {
    const auto candidates = collectCandidatesAt(viewPos);
    if (candidates.empty()) {
        return;
    }

    QMenu menu(this);

    if (selectedMetadataIndex_ >= 0) {
        QAction* clearAction = menu.addAction(QStringLiteral("Снять выделение"));
        connect(clearAction, &QAction::triggered, this, [this]() {
            clearPersistentHighlight();
        });
        menu.addSeparator();
    }

    for (const auto& candidate : candidates) {
        QMenu* itemMenu = menu.addMenu(candidate.title);

        QAction* selectAction = itemMenu->addAction(QStringLiteral("Выделить"));
        connect(selectAction, &QAction::triggered, this, [this, idx = candidate.metadataIndex]() {
            selectMetadataIndex(idx);
        });

        QAction* propsAction = itemMenu->addAction(QStringLiteral("Свойства"));
        connect(propsAction, &QAction::triggered, this, [this, idx = candidate.metadataIndex]() {
            selectMetadataIndex(idx);
            showPropertiesForMetadataIndex(idx);
        });
    }

    menu.exec(viewport()->mapToGlobal(viewPos));
}

void GdsSceneView::applyColorToItem(QGraphicsItem* item, const QColor& color) {
    if (item == nullptr) {
        return;
    }

    if (auto* polygon = dynamic_cast<HoverPolygonItem*>(item)) {
        QPen pen(color.darker(130));
        pen.setCosmetic(true);
        pen.setWidthF(1.0);
        QBrush brush(QColor(color.red(), color.green(), color.blue(), 90), Qt::SolidPattern);
        polygon->setNormalStyle(pen, brush);
        return;
    }

    if (auto* path = dynamic_cast<HoverPathItem*>(item)) {
        QPen pen = path->pen();
        pen.setColor(color);
        path->setNormalStyle(pen);
        return;
    }

    if (auto* text = qgraphicsitem_cast<QGraphicsSimpleTextItem*>(item)) {
        text->setBrush(QBrush(color));
        return;
    }

    if (auto* ellipse = qgraphicsitem_cast<QGraphicsEllipseItem*>(item)) {
        QPen pen = ellipse->pen();
        pen.setColor(color);
        ellipse->setPen(pen);

        if (ellipse->brush().style() != Qt::NoBrush) {
            ellipse->setBrush(QBrush(color));
        }
    }
}

void GdsSceneView::setMetadataHighlighted(int index, bool highlighted) {
    const auto it = metadataItems_.find(index);
    if (it == metadataItems_.end()) {
        return;
    }

    for (QGraphicsItem* item : it->second) {
        if (auto* polygon = dynamic_cast<HoverPolygonItem*>(item)) {
            polygon->setPersistentHighlight(highlighted);
            continue;
        }

        if (auto* path = dynamic_cast<HoverPathItem*>(item)) {
            path->setPersistentHighlight(highlighted);
            continue;
        }

        applySelectedStyleToGenericItem(item, highlighted);
    }
}

void GdsSceneView::clearPersistentHighlight() {
    if (selectedMetadataIndex_ < 0) {
        return;
    }

    setMetadataHighlighted(selectedMetadataIndex_, false);
    selectedMetadataIndex_ = -1;
    scene_.update();
    viewport()->update();

    emit selectedElementChanged(QString());
}

void GdsSceneView::selectMetadataIndex(int index) {
    if (index < 0 || index >= static_cast<int>(metadataStorage_.size())) {
        return;
    }

    if (selectedMetadataIndex_ == index) {
        setMetadataHighlighted(index, true);
        scene_.update();
        viewport()->update();
        emit selectedElementChanged(makeSelectedText(*metadataStorage_[index]));
        return;
    }

    clearPersistentHighlight();
    selectedMetadataIndex_ = index;
    setMetadataHighlighted(selectedMetadataIndex_, true);
    scene_.update();
    viewport()->update();

    emit selectedElementChanged(makeSelectedText(*metadataStorage_[selectedMetadataIndex_]));
}

SceneElementInfo GdsSceneView::makeInfo(const gds::Boundary& boundary) {
    SceneElementInfo info = makeCommonInfo(boundary.common);
    info.elementType = QStringLiteral("Boundary");
    info.layer = boundary.layer;
    info.subtypeName = QStringLiteral("Datatype");
    info.subtypeValue = boundary.datatype;
    info.points = boundary.xy;
    return info;
}

SceneElementInfo GdsSceneView::makeInfo(const gds::Box& box) {
    SceneElementInfo info = makeCommonInfo(box.common);
    info.elementType = QStringLiteral("Box");
    info.layer = box.layer;
    info.subtypeName = QStringLiteral("Box type");
    info.subtypeValue = box.boxtype;
    info.points = box.xy;
    return info;
}

SceneElementInfo GdsSceneView::makeInfo(const gds::Path& path) {
    SceneElementInfo info = makeCommonInfo(path.common);
    info.elementType = QStringLiteral("Path");
    info.layer = path.layer;
    info.subtypeName = QStringLiteral("Datatype");
    info.subtypeValue = path.datatype;
    info.points = path.xy;
    return info;
}

SceneElementInfo GdsSceneView::makeInfo(const gds::Text& text) {
    SceneElementInfo info = makeCommonInfo(text.common);
    info.elementType = QStringLiteral("Text");
    info.layer = text.layer;
    info.subtypeName = QStringLiteral("Text type");
    info.subtypeValue = text.texttype;
    info.primaryText = QString::fromStdString(text.string);
    info.points.push_back(text.xy);
    return info;
}

SceneElementInfo GdsSceneView::makeInfo(const gds::Node& node) {
    SceneElementInfo info = makeCommonInfo(node.common);
    info.elementType = QStringLiteral("Node");
    info.layer = node.layer;
    info.subtypeName = QStringLiteral("Node type");
    info.subtypeValue = node.nodetype;
    info.points = node.xy;
    return info;
}

SceneElementInfo GdsSceneView::makeInfo(const gds::SRef& sref) {
    SceneElementInfo info = makeCommonInfo(sref.common);
    info.elementType = QStringLiteral("SRef");
    info.primaryText = QString::fromStdString(sref.sname);
    info.points.push_back(sref.xy);
    return info;
}

SceneElementInfo GdsSceneView::makeInfo(const gds::ARef& aref) {
    SceneElementInfo info = makeCommonInfo(aref.common);
    info.elementType = QStringLiteral("ARef");
    info.primaryText = QString::fromStdString(aref.sname);
    info.points.push_back(aref.origin);
    info.points.push_back(aref.columnVector);
    info.points.push_back(aref.rowVector);
    return info;
}
