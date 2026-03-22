#include "HoverPolygonItem.hpp"

#include <QGraphicsSceneHoverEvent>

#include <algorithm>

HoverPolygonItem::HoverPolygonItem(const QPolygonF& polygon, QGraphicsItem* parent)
    : QGraphicsPolygonItem(polygon, parent) {
    setAcceptHoverEvents(true);
    setCursor(QCursor(Qt::ArrowCursor));
}

void HoverPolygonItem::setNormalStyle(const QPen& pen, const QBrush& brush) {
    normalPen_ = pen;
    normalBrush_ = brush;

    hoverPen_ = normalPen_;
    hoverBrush_ = normalBrush_;
    hoverPen_.setColor(normalPen_.color().lighter(200));

    if (normalBrush_.style() != Qt::NoBrush) {
        QColor c = normalBrush_.color().lighter(210);
        c.setAlpha(std::min(255, c.alpha() + 80));
        hoverBrush_.setColor(c);
    }

    selectedPen_ = normalPen_;
    selectedBrush_ = normalBrush_;
    //selectedPen_.setColor(normalPen_.color().lighter(250));
    selectedPen_.setColor(QColor(0, 0, 0));

    selectedPen_.setWidthF(std::max<qreal>(4.0, normalPen_.widthF() + 3.0));

    if (normalBrush_.style() != Qt::NoBrush) {
        QColor c = normalBrush_.color().lighter(155);
        c.setAlpha(std::min(255, c.alpha() + 120));
        selectedBrush_.setColor(c);
    }

    updateVisualState();
}

void HoverPolygonItem::setPersistentHighlight(bool enabled) {
    if (selected_ == enabled) {
        return;
    }

    selected_ = enabled;
    updateVisualState();
}

//void HoverPolygonItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
//    setCursor(QCursor(Qt::ArrowCursor));
//    hovered_ = true;
//    updateVisualState();
//    QGraphicsPolygonItem::hoverEnterEvent(event);
//}
//
//void HoverPolygonItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
//    setCursor(QCursor(Qt::ArrowCursor));
//    hovered_ = false;
//    updateVisualState();
//    QGraphicsPolygonItem::hoverLeaveEvent(event);
//}

void HoverPolygonItem::updateVisualState() {
    if (selected_) {
        setPen(selectedPen_);
        setBrush(selectedBrush_);
        setZValue(1000.0);
        return;
    }

  /*  if (hovered_) {
        setPen(hoverPen_);
        setBrush(hoverBrush_);
        return;
    }*/

    setPen(normalPen_);
    setBrush(normalBrush_);
    setZValue(0.0);
}
