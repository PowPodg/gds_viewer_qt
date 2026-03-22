#pragma once

#include "ElementPropertiesDialog.hpp"
#include "GdsStructure.hpp"

#include <QColor>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPoint>

#include <map>
#include <memory>
#include <vector>

class QAction;
class QGraphicsItem;
class QMouseEvent;
class QWheelEvent;

class GdsSceneView final : public QGraphicsView {
    Q_OBJECT
public:
    struct LayerInfo {
        int layer{};
        int elementCount{};
        QColor color;
    };


    explicit GdsSceneView(QWidget* parent = nullptr);

    void clearData();
    void beginIncrementalBuild();
    void appendElement(const gds::GdsElement& element);
    void finalizeIncrementalBuild(bool fitView);
    void loadStructure(const gds::GdsStructure& structure);

    std::vector<LayerInfo> layerInfos() const;
    void setLayerVisible(int layer, bool visible);
    void setLayerColor(int layer, const QColor& color);
    QColor layerColor(int layer) const;
    bool hasContent() const;
    void fitAll();

signals:
    void selectedElementChanged(const QString& text);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    struct LayerBucket {
        int elementCount{};
        QColor color;
        std::vector<QGraphicsItem*> items;
    };

    struct CandidateItem {
        int metadataIndex = -1;
        QString title;
        double area = 0.0;
    };

    QGraphicsScene scene_;
    std::map<int, LayerBucket> layers_;
    std::vector<std::unique_ptr<SceneElementInfo>> metadataStorage_;
    std::map<int, std::vector<QGraphicsItem*>> metadataItems_;
    QPoint pressPos_;
    int selectedMetadataIndex_ = -1;

    void addBoundary(const gds::Boundary& boundary);
    void addBox(const gds::Box& box);
    void addPath(const gds::Path& path);
    void addTextMarker(const gds::Text& text);
    void addNode(const gds::Node& node);
    void addSRef(const gds::SRef& sref);
    void addARef(const gds::ARef& aref);

    void registerItem(int layer, QGraphicsItem* item, const SceneElementInfo& info);
    int storeMetadata(const SceneElementInfo& info);
    void showPropertiesForMetadataIndex(int index);
    void showContextMenuAt(const QPoint& viewPos);

  
    std::vector<CandidateItem> collectCandidatesAt(const QPoint& viewPos) const;
    QString makeCandidateTitle(const SceneElementInfo& info) const;
    QString makeSelectedText(const SceneElementInfo& info) const;

    void applyColorToItem(QGraphicsItem* item, const QColor& color);
    void setMetadataHighlighted(int index, bool highlighted);
    void clearPersistentHighlight();
    void selectMetadataIndex(int index);

    static SceneElementInfo makeInfo(const gds::Boundary& boundary);
    static SceneElementInfo makeInfo(const gds::Box& box);
    static SceneElementInfo makeInfo(const gds::Path& path);
    static SceneElementInfo makeInfo(const gds::Text& text);
    static SceneElementInfo makeInfo(const gds::Node& node);
    static SceneElementInfo makeInfo(const gds::SRef& sref);
    static SceneElementInfo makeInfo(const gds::ARef& aref);
    static QColor defaultColorForLayer(int layer);
};
