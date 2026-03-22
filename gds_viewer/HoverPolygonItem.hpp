#pragma once

#include <QBrush>
#include <QCursor>
#include <QGraphicsPolygonItem>
#include <QPen>

class HoverPolygonItem final : public QGraphicsPolygonItem {
public:
    explicit HoverPolygonItem(const QPolygonF& polygon, QGraphicsItem* parent = nullptr);

    void setNormalStyle(const QPen& pen, const QBrush& brush);
    void setPersistentHighlight(bool enabled);
    bool persistentHighlight() const { return selected_; }

protected:
   /* void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;*/

private:
    void updateVisualState();

    QPen normalPen_;
    QBrush normalBrush_;
    QPen hoverPen_;
    QBrush hoverBrush_;
    QPen selectedPen_;
    QBrush selectedBrush_;
    bool hovered_ = false;
    bool selected_ = false;
};
