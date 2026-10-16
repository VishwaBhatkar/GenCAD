#include <QTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <cmath>

#include "geometry/ArcEntity.h"
#include "geometry/PolylineEntity.h"
#include "geometry/TextEntity.h"
#include "geometry/DimensionEntity.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"

#include "layers/CadLayer.h"
#include "layers/LayerManager.h"
#include "document/CadDocument.h"

#include "snap/SnapManager.h"

#include "commands/RotateEntityCommand.h"
#include "commands/MirrorEntityCommand.h"
#include "commands/CopyEntityCommand.h"

#include "io/DxfFile.h"
#include "io/GencadFile.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Phase2Tests : public QObject {
    Q_OBJECT

private slots:
    // Geometry tests
    void testArcGeometry();
    void testPolylineGeometry();
    void testTextGeometry();
    void testDimensionGeometry();
    void testRotatedRectangleGeometry();

    // Layers tests
    void testLayerCreationAndDefaults();
    void testLayerManagerOperations();
    void testDocumentLayerAssignment();

    // Snap tests
    void testSnapManagerEndpointAndMidpoint();
    void testSnapManagerCircleAndQuadrant();
    void testSnapManagerGridSnap();
    void testSnapIgnoresHiddenLayers();

    // Commands tests
    void testRotateCommand();
    void testMirrorCommandInPlace();
    void testMirrorCommandWithCopy();
    void testCopyCommand();

    // File I/O tests
    void testDxfExportAndImportRoundTrip();
    void testGencadPhase2JsonRoundTrip();
};

void Phase2Tests::testArcGeometry() {
    ArcEntity arc(QPointF(100.0, 100.0), 50.0, 0.0, 90.0);
    QCOMPARE(arc.center(), QPointF(100.0, 100.0));
    QCOMPARE(arc.radius(), 50.0);
    QCOMPARE(arc.startAngle(), 0.0);
    QCOMPARE(arc.sweepAngle(), 90.0);
    QCOMPARE(arc.endAngle(), 90.0);

    // Start point: center + (50 * cos(0), 50 * sin(0)) = (150, 100)
    QPointF start = arc.startPoint();
    QVERIFY(std::abs(start.x() - 150.0) < 1e-3);
    QVERIFY(std::abs(start.y() - 100.0) < 1e-3);

    // End point: center + (50 * cos(90 deg), 50 * sin(90 deg)) = (100, 150)
    QPointF end = arc.endPoint();
    QVERIFY(std::abs(end.x() - 100.0) < 1e-3);
    QVERIFY(std::abs(end.y() - 150.0) < 1e-3);

    // Mid point: at 45 deg
    QPointF mid = arc.midPoint();
    double expectedMidX = 100.0 + 50.0 * std::cos(45.0 * M_PI / 180.0);
    double expectedMidY = 100.0 + 50.0 * std::sin(45.0 * M_PI / 180.0);
    QVERIFY(std::abs(mid.x() - expectedMidX) < 1e-3);
    QVERIFY(std::abs(mid.y() - expectedMidY) < 1e-3);

    // Arc length: r * theta = 50 * (pi / 2)
    QVERIFY(std::abs(arc.arcLength() - 50.0 * (M_PI * 0.5)) < 1e-3);

    // Hit test on the arc perimeter
    QVERIFY(arc.hitTest(QPointF(150.0, 100.0), 2.0));
    // Point far away
    QVERIFY(!arc.hitTest(QPointF(300.0, 300.0), 2.0));

    // Rotation test: rotate 90 degrees around center
    arc.rotate(QPointF(100.0, 100.0), M_PI * 0.5);
    QVERIFY(std::abs(arc.startAngle() - 90.0) < 1e-2);

    // JSON serialization
    QJsonObject json = arc.toJson();
    ArcEntity deserialized;
    QVERIFY(deserialized.fromJson(json));
    QCOMPARE(deserialized.radius(), arc.radius());
    QCOMPARE(deserialized.sweepAngle(), arc.sweepAngle());
}

void Phase2Tests::testPolylineGeometry() {
    std::vector<QPointF> pts = {
        QPointF(0.0, 0.0),
        QPointF(100.0, 0.0),
        QPointF(100.0, 50.0)
    };
    PolylineEntity poly(pts, false);
    QCOMPARE(poly.pointCount(), static_cast<size_t>(3));
    QCOMPARE(poly.isClosed(), false);

    // Total length: 100 + 50 = 150
    QVERIFY(std::abs(poly.totalLength() - 150.0) < 1e-3);

    // Closed polyline length: 150 + hypot(100, 50)
    poly.setClosed(true);
    QVERIFY(std::abs(poly.totalLength() - (150.0 + std::hypot(100.0, 50.0))) < 1e-3);

    // Move polyline
    poly.move(10.0, 20.0);
    QCOMPARE(poly.pointAt(0), QPointF(10.0, 20.0));
    QCOMPARE(poly.pointAt(1), QPointF(110.0, 20.0));

    // Snap points: should contain vertices and midpoints
    auto snaps = poly.snapPoints();
    QVERIFY(snaps.size() >= 5);

    // JSON round-trip
    QJsonObject json = poly.toJson();
    PolylineEntity deserialized;
    QVERIFY(deserialized.fromJson(json));
    QCOMPARE(deserialized.pointCount(), poly.pointCount());
    QCOMPARE(deserialized.isClosed(), true);
}

void Phase2Tests::testTextGeometry() {
    TextEntity text(QPointF(50.0, 80.0), QStringLiteral("GenCAD CAD"), 12.0, 0.0);
    QCOMPARE(text.position(), QPointF(50.0, 80.0));
    QCOMPARE(text.text(), QStringLiteral("GenCAD CAD"));
    QCOMPARE(text.height(), 12.0);
    QCOMPARE(text.rotationDeg(), 0.0);

    // Move text
    text.move(15.0, -10.0);
    QCOMPARE(text.position(), QPointF(65.0, 70.0));

    // Rotate text
    text.rotate(QPointF(65.0, 70.0), M_PI * 0.25); // 45 deg
    QVERIFY(std::abs(text.rotationDeg() - 45.0) < 1e-2);

    // Snap points
    auto snaps = text.snapPoints();
    QCOMPARE(snaps.size(), static_cast<size_t>(1));
    QCOMPARE(snaps[0], text.position());

    // JSON round-trip
    QJsonObject json = text.toJson();
    TextEntity deserialized;
    QVERIFY(deserialized.fromJson(json));
    QCOMPARE(deserialized.text(), QStringLiteral("GenCAD CAD"));
    QCOMPARE(deserialized.height(), 12.0);
}

void Phase2Tests::testDimensionGeometry() {
    DimensionEntity dim(QPointF(0.0, 0.0), QPointF(100.0, 0.0), QPointF(50.0, 25.0));
    QCOMPARE(dim.p1(), QPointF(0.0, 0.0));
    QCOMPARE(dim.p2(), QPointF(100.0, 0.0));
    QCOMPARE(dim.dimPoint(), QPointF(50.0, 25.0));

    // Measured value should be 100.0
    QVERIFY(std::abs(dim.measuredValue() - 100.0) < 1e-4);
    QCOMPARE(dim.dimensionText(), QStringLiteral("100.00"));

    // Rotate dimension
    dim.rotate(QPointF(0.0, 0.0), M_PI * 0.5); // 90 degrees
    QVERIFY(std::abs(dim.p2().x() - 0.0) < 1e-3);
    QVERIFY(std::abs(dim.p2().y() - 100.0) < 1e-3);

    // JSON round-trip
    QJsonObject json = dim.toJson();
    DimensionEntity deserialized;
    QVERIFY(deserialized.fromJson(json));
    QCOMPARE(deserialized.dimensionText(), QStringLiteral("100.00"));
}

void Phase2Tests::testRotatedRectangleGeometry() {
    RectangleEntity rect(QPointF(0.0, 0.0), 100.0, 50.0);
    QCOMPARE(rect.corners()[0], QPointF(0.0, 0.0));
    QCOMPARE(rect.corners()[1], QPointF(100.0, 0.0));
    QCOMPARE(rect.corners()[2], QPointF(100.0, 50.0));
    QCOMPARE(rect.corners()[3], QPointF(0.0, 50.0));

    // Rotate 90 degrees around origin (0, 0)
    rect.rotate(QPointF(0.0, 0.0), M_PI * 0.5);
    // (100, 0) rotated by 90 deg -> (0, 100)
    QVERIFY(std::abs(rect.corners()[1].x() - 0.0) < 1e-3);
    QVERIFY(std::abs(rect.corners()[1].y() - 100.0) < 1e-3);

    // Snap points: 4 corners + 4 midpoints + 1 center = 9 snap points
    auto snaps = rect.snapPoints();
    QCOMPARE(snaps.size(), static_cast<size_t>(9));
}

void Phase2Tests::testLayerCreationAndDefaults() {
    LayerManager mgr;
    // Default layer "0" must exist
    QVERIFY(mgr.findLayer(QStringLiteral("0")) != nullptr);
    QCOMPARE(mgr.activeLayerName(), QStringLiteral("0"));

    // Cannot remove layer "0"
    QVERIFY(!mgr.removeLayer(QStringLiteral("0")));
    QCOMPARE(mgr.layers().size(), static_cast<size_t>(1));

    // Add new layer
    QVERIFY(mgr.addLayer(QStringLiteral("Walls"), QColor(255, 0, 0), 2.0));
    QCOMPARE(mgr.layers().size(), static_cast<size_t>(2));

    // Cannot add duplicate layer name
    QVERIFY(!mgr.addLayer(QStringLiteral("Walls")));

    // Set active layer
    mgr.setActiveLayer(QStringLiteral("Walls"));
    QCOMPARE(mgr.activeLayerName(), QStringLiteral("Walls"));

    // Clear restores default layer "0"
    mgr.clear();
    QCOMPARE(mgr.layers().size(), static_cast<size_t>(1));
    QCOMPARE(mgr.activeLayerName(), QStringLiteral("0"));
}

void Phase2Tests::testLayerManagerOperations() {
    LayerManager mgr;
    mgr.addLayer(QStringLiteral("Mechanical"), QColor(0, 255, 0), 1.5);
    auto layer = mgr.findLayer(QStringLiteral("Mechanical"));
    QVERIFY(layer != nullptr);
    QVERIFY(layer->isVisible());
    QVERIFY(!layer->isLocked());

    mgr.setLayerVisible(QStringLiteral("Mechanical"), false);
    QVERIFY(!layer->isVisible());

    mgr.setLayerLocked(QStringLiteral("Mechanical"), true);
    QVERIFY(layer->isLocked());

    mgr.setLayerColor(QStringLiteral("Mechanical"), QColor(0, 0, 255));
    QCOMPARE(layer->color(), QColor(0, 0, 255));

    // JSON serialization
    QJsonArray json = mgr.toJson();
    LayerManager mgr2;
    QVERIFY(mgr2.fromJson(json));
    QVERIFY(mgr2.findLayer(QStringLiteral("Mechanical")) != nullptr);
    QCOMPARE(mgr2.findLayer(QStringLiteral("Mechanical"))->isVisible(), false);
    QCOMPARE(mgr2.findLayer(QStringLiteral("Mechanical"))->isLocked(), true);
}

void Phase2Tests::testDocumentLayerAssignment() {
    CadDocument doc;
    doc.layerManager()->addLayer(QStringLiteral("Doors"));
    doc.layerManager()->setActiveLayer(QStringLiteral("Doors"));

    auto line = std::make_shared<LineEntity>(QPointF(0, 0), QPointF(10, 10));
    doc.addEntity(line);

    // Entity automatically assigned active layer
    QCOMPARE(line->layer(), QStringLiteral("Doors"));
}

void Phase2Tests::testSnapManagerEndpointAndMidpoint() {
    CadDocument doc;
    auto line = std::make_shared<LineEntity>(QPointF(0.0, 0.0), QPointF(100.0, 0.0));
    doc.addEntity(line);

    SnapManager sm;
    sm.setSnapEnabled(true);
    sm.setSnapTolerance(15.0);

    // Near p1 (0, 0)
    SnapResult r1 = sm.findSnap(QPointF(3.0, 2.0), &doc, 15.0);
    QVERIFY(r1.snapped);
    QCOMPARE(r1.type, SnapType::Endpoint);
    QCOMPARE(r1.point, QPointF(0.0, 0.0));

    // Near midpoint (50, 0)
    SnapResult rMid = sm.findSnap(QPointF(52.0, -1.0), &doc, 15.0);
    QVERIFY(rMid.snapped);
    QCOMPARE(rMid.type, SnapType::Midpoint);
    QCOMPARE(rMid.point, QPointF(50.0, 0.0));

    // Far away: no snap
    SnapResult rFar = sm.findSnap(QPointF(200.0, 200.0), &doc, 15.0);
    QVERIFY(!rFar.snapped);
}

void Phase2Tests::testSnapManagerCircleAndQuadrant() {
    CadDocument doc;
    auto circle = std::make_shared<CircleEntity>(QPointF(50.0, 50.0), 30.0);
    doc.addEntity(circle);

    SnapManager sm;
    sm.setSnapEnabled(true);

    // Near center (50, 50)
    SnapResult rCenter = sm.findSnap(QPointF(51.0, 49.0), &doc, 15.0);
    QVERIFY(rCenter.snapped);
    QCOMPARE(rCenter.type, SnapType::Center);
    QCOMPARE(rCenter.point, QPointF(50.0, 50.0));

    // Near right quadrant (80, 50)
    SnapResult rQuad = sm.findSnap(QPointF(82.0, 50.0), &doc, 15.0);
    QVERIFY(rQuad.snapped);
    QCOMPARE(rQuad.type, SnapType::Quadrant);
    QCOMPARE(rQuad.point, QPointF(80.0, 50.0));
}

void Phase2Tests::testSnapManagerGridSnap() {
    CadDocument doc;
    SnapManager sm;
    sm.setSnapEnabled(false);
    sm.setGridSnapEnabled(true);
    sm.setGridSize(10.0);

    // Point near (20, 30)
    SnapResult r = sm.findSnap(QPointF(21.2, 29.1), &doc, 5.0);
    QVERIFY(r.snapped);
    QCOMPARE(r.type, SnapType::Grid);
    QCOMPARE(r.point, QPointF(20.0, 30.0));
}

void Phase2Tests::testSnapIgnoresHiddenLayers() {
    CadDocument doc;
    doc.layerManager()->addLayer(QStringLiteral("HiddenLayer"));
    auto line = std::make_shared<LineEntity>(QPointF(0, 0), QPointF(100, 0));
    line->setLayer(QStringLiteral("HiddenLayer"));
    doc.addEntity(line);

    SnapManager sm;
    sm.setSnapEnabled(true);

    // Visible: should snap
    SnapResult r1 = sm.findSnap(QPointF(2, 2), &doc, 15.0);
    QVERIFY(r1.snapped);

    // Hide layer: should not snap!
    doc.layerManager()->setLayerVisible(QStringLiteral("HiddenLayer"), false);
    SnapResult r2 = sm.findSnap(QPointF(2, 2), &doc, 15.0);
    QVERIFY(!r2.snapped);
}

void Phase2Tests::testRotateCommand() {
    CadDocument doc;
    auto line = std::make_shared<LineEntity>(QPointF(0, 0), QPointF(100, 0));
    doc.addEntity(line);

    auto cmd = new RotateEntityCommand(&doc, {line->id()}, QPointF(0, 0), M_PI * 0.5);
    cmd->redo();

    // Line endpoint should now be (0, 100)
    QVERIFY(std::abs(line->p2().x() - 0.0) < 1e-3);
    QVERIFY(std::abs(line->p2().y() - 100.0) < 1e-3);

    // Undo: restores original (100, 0)
    cmd->undo();
    QVERIFY(std::abs(line->p2().x() - 100.0) < 1e-3);
    QVERIFY(std::abs(line->p2().y() - 0.0) < 1e-3);

    delete cmd;
}

void Phase2Tests::testMirrorCommandInPlace() {
    CadDocument doc;
    auto line = std::make_shared<LineEntity>(QPointF(10, 20), QPointF(50, 20));
    doc.addEntity(line);

    // Mirror across vertical axis x = 0 (line from (0, 0) to (0, 100))
    auto cmd = new MirrorEntityCommand(&doc, {line->id()}, QPointF(0, 0), QPointF(0, 100), false);
    cmd->redo();

    // p1 x reflected: -10, p2 x reflected: -50
    QVERIFY(std::abs(line->p1().x() - (-10.0)) < 1e-3);
    QVERIFY(std::abs(line->p2().x() - (-50.0)) < 1e-3);

    // Undo: restores original
    cmd->undo();
    QVERIFY(std::abs(line->p1().x() - 10.0) < 1e-3);
    QVERIFY(std::abs(line->p2().x() - 50.0) < 1e-3);

    delete cmd;
}

void Phase2Tests::testMirrorCommandWithCopy() {
    CadDocument doc;
    auto line = std::make_shared<LineEntity>(QPointF(10, 0), QPointF(50, 0));
    doc.addEntity(line);
    QCOMPARE(doc.entityCount(), static_cast<size_t>(1));

    // Mirror copy across Y axis
    auto cmd = new MirrorEntityCommand(&doc, {line->id()}, QPointF(0, 0), QPointF(0, 100), true);
    cmd->redo();
    QCOMPARE(doc.entityCount(), static_cast<size_t>(2));

    // Undo: removes the copy
    cmd->undo();
    QCOMPARE(doc.entityCount(), static_cast<size_t>(1));

    delete cmd;
}

void Phase2Tests::testCopyCommand() {
    CadDocument doc;
    auto line = std::make_shared<LineEntity>(QPointF(0, 0), QPointF(10, 10));
    doc.addEntity(line);

    auto cmd = new CopyEntityCommand(&doc, {line->id()}, 50.0, 50.0);
    cmd->redo();
    QCOMPARE(doc.entityCount(), static_cast<size_t>(2));

    auto copies = cmd->createdCopies();
    QCOMPARE(copies.size(), static_cast<size_t>(1));
    const auto* copyLine = static_cast<const LineEntity*>(copies[0].get());
    QCOMPARE(copyLine->p1(), QPointF(50.0, 50.0));
    QCOMPARE(copyLine->p2(), QPointF(60.0, 60.0));

    // Undo
    cmd->undo();
    QCOMPARE(doc.entityCount(), static_cast<size_t>(1));

    delete cmd;
}

void Phase2Tests::testDxfExportAndImportRoundTrip() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString dxfPath = tempDir.filePath(QStringLiteral("test_drawing.dxf"));

    CadDocument doc;
    doc.layerManager()->addLayer(QStringLiteral("Walls"), QColor(255, 0, 0));
    doc.layerManager()->addLayer(QStringLiteral("Annotation"), QColor(0, 255, 255));

    // Add Line
    auto line = std::make_shared<LineEntity>(QPointF(10, 20), QPointF(100, 20));
    line->setLayer(QStringLiteral("Walls"));
    doc.addEntity(line);

    // Add Circle
    auto circle = std::make_shared<CircleEntity>(QPointF(50, 50), 25);
    circle->setLayer(QStringLiteral("Walls"));
    doc.addEntity(circle);

    // Add Arc
    auto arc = std::make_shared<ArcEntity>(QPointF(150, 150), 40, 30, 90);
    arc->setLayer(QStringLiteral("Walls"));
    doc.addEntity(arc);

    // Add Polyline
    std::vector<QPointF> polyPts = { QPointF(0, 0), QPointF(50, 0), QPointF(50, 50) };
    auto poly = std::make_shared<PolylineEntity>(polyPts, true);
    poly->setLayer(QStringLiteral("Walls"));
    doc.addEntity(poly);

    // Add Text
    auto text = std::make_shared<TextEntity>(QPointF(20, 200), QStringLiteral("GenCAD DXF"), 15.0, 45.0);
    text->setLayer(QStringLiteral("Annotation"));
    doc.addEntity(text);

    // Add Dimension
    auto dim = std::make_shared<DimensionEntity>(QPointF(0, 0), QPointF(100, 0), QPointF(50, 30));
    dim->setLayer(QStringLiteral("Annotation"));
    doc.addEntity(dim);

    // Export to DXF
    QString err;
    QVERIFY2(DxfFile::save(dxfPath, &doc, &err), qPrintable(err));

    QFileInfo info(dxfPath);
    QVERIFY(info.exists());
    QVERIFY(info.size() > 100);

    // Import from DXF into fresh document
    CadDocument importedDoc;
    QVERIFY2(DxfFile::load(dxfPath, &importedDoc, &err), qPrintable(err));

    // Verify layers reconstructed
    QVERIFY(importedDoc.layerManager()->findLayer(QStringLiteral("Walls")) != nullptr);
    QVERIFY(importedDoc.layerManager()->findLayer(QStringLiteral("Annotation")) != nullptr);

    // Verify entity count
    QCOMPARE(importedDoc.entityCount(), static_cast<size_t>(6));
}

void Phase2Tests::testGencadPhase2JsonRoundTrip() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString filePath = tempDir.filePath(QStringLiteral("test_v2.gencad"));

    CadDocument doc;
    doc.layerManager()->addLayer(QStringLiteral("Architecture"), QColor(255, 128, 0));
    doc.layerManager()->setActiveLayer(QStringLiteral("Architecture"));

    auto arc = std::make_shared<ArcEntity>(QPointF(50, 50), 30, 0, 180);
    doc.addEntity(arc);

    auto text = std::make_shared<TextEntity>(QPointF(10, 10), QStringLiteral("RoundTrip"), 12.0, 0.0);
    doc.addEntity(text);

    QString err;
    QVERIFY(GencadFile::save(filePath, &doc, &err));

    CadDocument loadedDoc;
    QVERIFY(GencadFile::load(filePath, &loadedDoc, &err));

    QCOMPARE(loadedDoc.entityCount(), static_cast<size_t>(2));
    QCOMPARE(loadedDoc.layerManager()->activeLayerName(), QStringLiteral("Architecture"));
    QVERIFY(loadedDoc.layerManager()->findLayer(QStringLiteral("Architecture")) != nullptr);
}

#include "Phase2Tests.moc"

QObject* createPhase2Tests() {
    return new Phase2Tests();
}
