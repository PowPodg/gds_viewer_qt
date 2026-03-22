#include "HoverPathItem.hpp"

#include <QGraphicsSceneHoverEvent>

#include <algorithm>

HoverPathItem::HoverPathItem(const QPainterPath& path, QGraphicsItem* parent)
    : QGraphicsPathItem(path, parent) {
   // setAcceptHoverEvents(true);
    setCursor(QCursor(Qt::ArrowCursor));
}

void HoverPathItem::setNormalStyle(const QPen& pen) {
    normalPen_ = pen;

    hoverPen_ = normalPen_;
    hoverPen_.setColor(normalPen_.color().lighter(220));
    hoverPen_.setWidthF(normalPen_.widthF() + 2.0);

    selectedPen_ = normalPen_;
    selectedPen_.setColor(normalPen_.color().lighter(255));
    selectedPen_.setWidthF(std::max<qreal>(3.0, normalPen_.widthF() + 4.0));

    updateVisualState();
}

void HoverPathItem::setPersistentHighlight(bool enabled) {
    if (selected_ == enabled) {
        return;
    }

    selected_ = enabled;
    updateVisualState();
}

//void HoverPathItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
//    setCursor(QCursor(Qt::ArrowCursor));
//    hovered_ = true;
//    updateVisualState();
//    QGraphicsPathItem::hoverEnterEvent(event);
//}
//
//void HoverPathItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
//    setCursor(QCursor(Qt::ArrowCursor));
//    hovered_ = false;
//    updateVisualState();
//    QGraphicsPathItem::hoverLeaveEvent(event);
//}

void HoverPathItem::updateVisualState() {
    if (selected_) {
        setPen(selectedPen_);
        setZValue(1000.0);
        return;
    }

    //if (hovered_) {
    //    setPen(hoverPen_);
    //    return;
    //}

    setPen(normalPen_);
    setZValue(0.0);
}
