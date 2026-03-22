#pragma once

#include <QCursor>
#include <QGraphicsPathItem>
#include <QPen>

class HoverPathItem final : public QGraphicsPathItem {
public:
    explicit HoverPathItem(const QPainterPath& path, QGraphicsItem* parent = nullptr);

    void setNormalStyle(const QPen& pen);
    void setPersistentHighlight(bool enabled);
    bool persistentHighlight() const { return selected_; }

protected:
  /*  void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;*/

private:
    void updateVisualState();

    QPen normalPen_;
    QPen hoverPen_;
    QPen selectedPen_;
    bool hovered_ = false;
    bool selected_ = false;
};
