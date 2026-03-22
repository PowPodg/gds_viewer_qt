#include "MainWindow.hpp"
#include "GdsSceneView.hpp"
#include "GdsStructure.hpp"
#include "GdsElements.hpp"

#include <QAction>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QTableWidget>
#include <QtTest/QtTest>

#include <algorithm>
#include <vector>

namespace {

gds::Boundary makeBoundary(
    std::int16_t layer,
    std::int16_t datatype,
    std::initializer_list<gds::Point> points)
{
    gds::Boundary b;
    b.layer = layer;
    b.datatype = datatype;
    b.xy.assign(points.begin(), points.end());
    return b;
}

gds::Path makePath(
    std::int16_t layer,
    std::int16_t datatype,
    std::optional<std::int32_t> width,
    std::initializer_list<gds::Point> points)
{
    gds::Path p;
    p.layer = layer;
    p.datatype = datatype;
    p.width = width;
    p.xy.assign(points.begin(), points.end());
    return p;
}

gds::GdsStructure makeTestStructure()
{
    gds::GdsStructure s;
    s.name = "viewer_test";

    s.addElement(makeBoundary(
        1, 0,
        {
            {0, 0},
            {100, 0},
            {100, 100},
            {0, 100},
            {0, 0}
        }));

    s.addElement(makeBoundary(
        2, 0,
        {
            {200, 0},
            {320, 0},
            {320, 80},
            {200, 80},
            {200, 0}
        }));

    s.addElement(makePath(
        3, 0, 10,
        {
            {0, 200},
            {150, 200}
        }));

    return s;
}

int visibleSceneItemCount(const GdsSceneView& view)
{
    if (view.scene() == nullptr) {
        return 0;
    }

    int count = 0;
    const auto items = view.scene()->items();
    for (QGraphicsItem* item : items) {
        if (item != nullptr && item->isVisible()) {
            ++count;
        }
    }
    return count;
}

int elementCountForLayer(const std::vector<GdsSceneView::LayerInfo>& infos, int layer)
{
    const auto it = std::find_if(infos.begin(), infos.end(),
        [layer](const auto& info) {
            return info.layer == layer;
        });

    return (it == infos.end()) ? -1 : it->elementCount;
}

} // namespace

class ViewerSmokeTests : public QObject
{
    Q_OBJECT

private slots:
    void sceneView_loadStructure_buildsSceneAndLayers();
    void sceneView_layerVisibilityAndColor_work();
    void mainWindow_constructs_requiredWidgets();
};

void ViewerSmokeTests::sceneView_loadStructure_buildsSceneAndLayers()
{
    const auto structure = makeTestStructure();

    GdsSceneView view;
    view.resize(800, 600);
    view.loadStructure(structure);

    QVERIFY(view.hasContent());
    QVERIFY(view.scene() != nullptr);
    QCOMPARE(visibleSceneItemCount(view), 3);

    const auto infos = view.layerInfos();
    QCOMPARE(static_cast<int>(infos.size()), 3);

    QCOMPARE(elementCountForLayer(infos, 1), 1);
    QCOMPARE(elementCountForLayer(infos, 2), 1);
    QCOMPARE(elementCountForLayer(infos, 3), 1);
}

void ViewerSmokeTests::sceneView_layerVisibilityAndColor_work()
{
    const auto structure = makeTestStructure();

    GdsSceneView view;
    view.resize(800, 600);
    view.loadStructure(structure);

    QCOMPARE(visibleSceneItemCount(view), 3);

    view.setLayerVisible(2, false);
    QCOMPARE(visibleSceneItemCount(view), 2);

    view.setLayerVisible(2, true);
    QCOMPARE(visibleSceneItemCount(view), 3);

    const QColor customColor(255, 0, 255);
    view.setLayerColor(2, customColor);
    QCOMPARE(view.layerColor(2), customColor);
}

void ViewerSmokeTests::mainWindow_constructs_requiredWidgets()
{
    MainWindow window;

    auto* view = window.findChild<GdsSceneView*>(QStringLiteral("graphicsView"));
    QVERIFY(view != nullptr);

    auto* layerTable = window.findChild<QTableWidget*>(QStringLiteral("layerTableWidget"));
    QVERIFY(layerTable != nullptr);

    auto* fitAction = window.findChild<QAction*>(QStringLiteral("actionFit"));
    QVERIFY(fitAction != nullptr);

    QVERIFY(!view->hasContent());
    QCOMPARE(layerTable->rowCount(), 0);
}

QTEST_MAIN(ViewerSmokeTests)
#include "ViewerSmokeTests.moc"