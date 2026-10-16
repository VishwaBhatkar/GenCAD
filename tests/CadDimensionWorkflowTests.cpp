#include <QTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <cmath>

#include "document/UnitSystem.h"
#include "document/PageSettings.h"
#include "document/CoordinateSystem.h"
#include "document/GridSettings.h"
#include "document/ConstraintEngine.h"
#include "document/CadDocument.h"

#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"
#include "geometry/ArcEntity.h"
#include "geometry/PolylineEntity.h"
#include "geometry/PolygonEntity.h"
#include "geometry/EllipseEntity.h"
#include "geometry/DimensionEntity.h"

#include "snap/SnapManager.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "tools/LineTool.h"
#include "tools/RectangleTool.h"
#include "tools/PolylineTool.h"
#include "tools/CircleTool.h"
#include "commands/CreateEntityCommand.h"
#include "commands/ModifyEntityPropertiesCommand.h"
#include "io/GencadFile.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class CadDimensionWorkflowTests : public QObject {
    Q_OBJECT

private slots:
    // 1. Units & Precision
    void testUnitConversionsLinear();
    void testUnitConversionsArea();
    void testPrecisionFormatting();
    void testUnitStringParsing();
    void testUnitSerializationRoundTrip();

    // 2. Page Setup & Limits
    void testPagePresetsDimensions();
    void testPageOrientationSwap();
    void testPageLimitsCalculation();
    void testPageSerializationRoundTrip();

    // 3. World Coordinate Math
    void testCoordinateDistanceAndAngle();
    void testPolarPointMath();
    void testOrthoSnapping();
    void testPolarRaySnapping();

    // 4. Polygon Geometry & Exact Formulas
    void testPolygonGeometryAreaPerimeter();
    void testPolygonSnapPoints();
    void testPolygonTransformations();
    void testPolygonSerialization();

    // 5. Ellipse Geometry & Exact Formulas
    void testEllipseGeometryAreaPerimeter();
    void testEllipseQuadrantsAndSnapPoints();
    void testEllipseTransformations();
    void testEllipseSerialization();

    // 6. Associative Dimensions
    void testDimensionTypesMeasuredValues();
    void testAssociativeDimensionTracksLine();
    void testAssociativeDimensionTracksCircle();
    void testDocumentSynchronizesAssociativeDimensions();

    // 7. Snapping Engine
    void testIntersectionSnap();
    void testPerpendicularSnap();
    void testTangentSnap();
    void testNearestSnap();
    void testPolarTrackingSnap();

    // 8. Full Persistence & Backward Compatibility
    void testFullWorkflowPersistenceRoundTrip();
    void testLegacyDrawingCompatibilityFallback();

    // 9. Section 29 Acceptance Tests
    void testAcceptance1_HorizontalLine();
    void testAcceptance2_VerticalLine();
    void testAcceptance3_F8();
    void testAcceptance4_OrthoOff();
    void testAcceptance5_ExactHorizontalDimension();
    void testAcceptance6_ExactVerticalDimension();
    void testAcceptance7_Rectangle();
    void testAcceptance8_Polyline();
    void testAcceptance9_GridSnapPlusOrtho();
    void testAcceptance10_ObjectSnapPlusOrtho();
    void testAcceptance11_DimensionEditing();
    void testAcceptance12_SaveLoad();
};

// ============================================================================
// 1. Units & Precision Tests
// ============================================================================

void CadDimensionWorkflowTests::testUnitConversionsLinear() {
    // Internal CAD world coordinate unit is strictly Millimeter (mm)
    UnitSettings mmSettings(UnitType::Millimeter);
    QCOMPARE(mmSettings.scaleToMm(), 1.0);
    QCOMPARE(mmSettings.scaleFromMm(), 1.0);
    QCOMPARE(mmSettings.fromInternal(250.0), 250.0);
    QCOMPARE(mmSettings.toInternal(250.0), 250.0);

    // Centimeters
    UnitSettings cmSettings(UnitType::Centimeter);
    QCOMPARE(cmSettings.scaleToMm(), 10.0);
    QCOMPARE(cmSettings.scaleFromMm(), 0.1);
    QCOMPARE(cmSettings.fromInternal(250.0), 25.0);
    QCOMPARE(cmSettings.toInternal(25.0), 250.0);

    // Meters
    UnitSettings mSettings(UnitType::Meter);
    QCOMPARE(mSettings.scaleToMm(), 1000.0);
    QCOMPARE(mSettings.scaleFromMm(), 0.001);
    QCOMPARE(mSettings.fromInternal(1500.0), 1.5);
    QCOMPARE(mSettings.toInternal(1.5), 1500.0);

    // Inches (1 in = 25.4 mm)
    UnitSettings inSettings(UnitType::Inch);
    QCOMPARE(inSettings.scaleToMm(), 25.4);
    QVERIFY(std::abs(inSettings.fromInternal(25.4) - 1.0) < 1e-9);
    QVERIFY(std::abs(inSettings.toInternal(1.0) - 25.4) < 1e-9);

    // Feet (1 ft = 304.8 mm)
    UnitSettings ftSettings(UnitType::Foot);
    QCOMPARE(ftSettings.scaleToMm(), 304.8);
    QVERIFY(std::abs(ftSettings.fromInternal(304.8) - 1.0) < 1e-9);
    QVERIFY(std::abs(ftSettings.toInternal(1.0) - 304.8) < 1e-9);

    // Custom: 1 yard = 914.4 mm
    UnitSettings customSettings(UnitType::Custom, 2, 914.4, QStringLiteral("yd"));
    QCOMPARE(customSettings.scaleToMm(), 914.4);
    QVERIFY(std::abs(customSettings.fromInternal(914.4) - 1.0) < 1e-9);
}

void CadDimensionWorkflowTests::testUnitConversionsArea() {
    // 1000 mm x 1000 mm = 1,000,000 sq mm
    double areaSqMm = 1000.0 * 1000.0;

    UnitSettings mmSettings(UnitType::Millimeter, 2);
    QCOMPARE(mmSettings.formatArea(areaSqMm, true), QStringLiteral("1000000.00 mm²"));

    UnitSettings cmSettings(UnitType::Centimeter, 2);
    QCOMPARE(cmSettings.formatArea(areaSqMm, true), QStringLiteral("10000.00 cm²"));

    UnitSettings mSettings(UnitType::Meter, 4);
    QCOMPARE(mSettings.formatArea(areaSqMm, true), QStringLiteral("1.0000 m²"));

    UnitSettings inSettings(UnitType::Inch, 2);
    double expectedSqIn = areaSqMm / (25.4 * 25.4);
    QCOMPARE(inSettings.formatArea(areaSqMm, false), QString::number(expectedSqIn, 'f', 2));
}

void CadDimensionWorkflowTests::testPrecisionFormatting() {
    UnitSettings settings(UnitType::Millimeter);

    // Precision 0 to 6 decimals
    settings.setPrecision(0);
    QCOMPARE(settings.formatLinear(123.456789, false), QStringLiteral("123"));

    settings.setPrecision(1);
    QCOMPARE(settings.formatLinear(123.456789, false), QStringLiteral("123.5"));

    settings.setPrecision(2);
    QCOMPARE(settings.formatLinear(123.456789, false), QStringLiteral("123.46"));

    settings.setPrecision(3);
    QCOMPARE(settings.formatLinear(123.456789, false), QStringLiteral("123.457"));

    settings.setPrecision(4);
    QCOMPARE(settings.formatLinear(123.456789, false), QStringLiteral("123.4568"));

    settings.setPrecision(6);
    QCOMPARE(settings.formatLinear(123.456789, false), QStringLiteral("123.456789"));
}

void CadDimensionWorkflowTests::testUnitStringParsing() {
    UnitSettings mmSettings(UnitType::Millimeter);
    bool ok = false;
    QCOMPARE(mmSettings.parseLinear(QStringLiteral("150.5"), &ok), 150.5);
    QVERIFY(ok);

    UnitSettings cmSettings(UnitType::Centimeter);
    QCOMPARE(cmSettings.parseLinear(QStringLiteral("12.5 cm"), &ok), 125.0);
    QVERIFY(ok);

    UnitSettings mSettings(UnitType::Meter);
    QCOMPARE(mSettings.parseLinear(QStringLiteral("2.5 m"), &ok), 2500.0);
    QVERIFY(ok);

    UnitSettings inSettings(UnitType::Inch);
    QVERIFY(std::abs(inSettings.parseLinear(QStringLiteral("2.0 in"), &ok) - 50.8) < 1e-4);
    QVERIFY(ok);

    // Angle parsing
    QCOMPARE(mmSettings.parseAngle(QStringLiteral("45.5°"), &ok), 45.5);
    QVERIFY(ok);
}

void CadDimensionWorkflowTests::testUnitSerializationRoundTrip() {
    UnitSettings units(UnitType::Inch, 4);
    QJsonObject json = units.toJson();

    UnitSettings loaded;
    QVERIFY(loaded.fromJson(json));
    QCOMPARE(loaded.type(), UnitType::Inch);
    QCOMPARE(loaded.precision(), 4);
    QCOMPARE(loaded.unitSuffix(), QStringLiteral("in"));
}

// ============================================================================
// 2. Page Setup & Limits Tests
// ============================================================================

void CadDimensionWorkflowTests::testPagePresetsDimensions() {
    // ISO Standard sheet sizes in Landscape orientation
    PageSettings a0(PageSize::A0, PageOrientation::Landscape);
    QCOMPARE(a0.widthMm(), 1189.0);
    QCOMPARE(a0.heightMm(), 841.0);

    PageSettings a1(PageSize::A1, PageOrientation::Landscape);
    QCOMPARE(a1.widthMm(), 841.0);
    QCOMPARE(a1.heightMm(), 594.0);

    PageSettings a2(PageSize::A2, PageOrientation::Landscape);
    QCOMPARE(a2.widthMm(), 594.0);
    QCOMPARE(a2.heightMm(), 420.0);

    PageSettings a3(PageSize::A3, PageOrientation::Landscape);
    QCOMPARE(a3.widthMm(), 420.0);
    QCOMPARE(a3.heightMm(), 297.0);

    PageSettings a4(PageSize::A4, PageOrientation::Landscape);
    QCOMPARE(a4.widthMm(), 297.0);
    QCOMPARE(a4.heightMm(), 210.0);

    PageSettings a5(PageSize::A5, PageOrientation::Landscape);
    QCOMPARE(a5.widthMm(), 210.0);
    QCOMPARE(a5.heightMm(), 148.0);
}

void CadDimensionWorkflowTests::testPageOrientationSwap() {
    PageSettings page(PageSize::A4, PageOrientation::Landscape);
    QCOMPARE(page.widthMm(), 297.0);
    QCOMPARE(page.heightMm(), 210.0);

    // Switch to Portrait
    page.setOrientation(PageOrientation::Portrait);
    QCOMPARE(page.widthMm(), 210.0);
    QCOMPARE(page.heightMm(), 297.0);

    // Custom page size orientation swap
    PageSettings custom(PageSize::Custom, PageOrientation::Landscape, 500.0, 300.0);
    QCOMPARE(custom.widthMm(), 500.0);
    QCOMPARE(custom.heightMm(), 300.0);
    custom.setOrientation(PageOrientation::Portrait);
    QCOMPARE(custom.widthMm(), 300.0);
    QCOMPARE(custom.heightMm(), 500.0);
}

void CadDimensionWorkflowTests::testPageLimitsCalculation() {
    PageSettings page(PageSize::A4, PageOrientation::Landscape);
    page.setDrawingLimits(10.0, 10.0, 287.0, 200.0);
    page.setEnforceLimits(true);

    QCOMPARE(page.minX(), 10.0);
    QCOMPARE(page.minY(), 10.0);
    QCOMPARE(page.maxX(), 287.0);
    QCOMPARE(page.maxY(), 200.0);
    QVERIFY(page.enforceLimits());

    QRectF limitsRect = page.limitsRect();
    QCOMPARE(limitsRect.x(), 10.0);
    QCOMPARE(limitsRect.y(), 10.0);
    QCOMPARE(limitsRect.width(), 277.0);
    QCOMPARE(limitsRect.height(), 190.0);
}

void CadDimensionWorkflowTests::testPageSerializationRoundTrip() {
    PageSettings page(PageSize::A3, PageOrientation::Portrait);
    page.setDrawingLimits(5.0, 5.0, 292.0, 415.0);
    page.setEnforceLimits(true);

    QJsonObject json = page.toJson();

    PageSettings loaded;
    QVERIFY(loaded.fromJson(json));
    QCOMPARE(loaded.pageSize(), PageSize::A3);
    QCOMPARE(loaded.orientation(), PageOrientation::Portrait);
    QCOMPARE(loaded.widthMm(), 297.0);
    QCOMPARE(loaded.heightMm(), 420.0);
    QCOMPARE(loaded.minX(), 5.0);
    QCOMPARE(loaded.maxX(), 292.0);
    QVERIFY(loaded.enforceLimits());
}

// ============================================================================
// 3. World Coordinate Math Tests
// ============================================================================

void CadDimensionWorkflowTests::testCoordinateDistanceAndAngle() {
    QPointF p1(0.0, 0.0);
    QPointF p2(30.0, 40.0);

    QCOMPARE(CoordinateSystem::distance(p1, p2), 50.0);
    QVERIFY(std::abs(CoordinateSystem::angleDeg(p1, QPointF(100.0, 0.0)) - 0.0) < 1e-4);
    QVERIFY(std::abs(CoordinateSystem::angleDeg(p1, QPointF(0.0, 100.0)) - 90.0) < 1e-4);
    QVERIFY(std::abs(CoordinateSystem::angleDeg(p1, QPointF(-100.0, 0.0)) - 180.0) < 1e-4);
    QVERIFY(std::abs(CoordinateSystem::angleDeg(p1, QPointF(0.0, -100.0)) - 270.0) < 1e-4);
}

void CadDimensionWorkflowTests::testPolarPointMath() {
    QPointF base(100.0, 100.0);
    double dist = 50.0;

    // 0 deg -> (150, 100)
    QPointF pt0 = CoordinateSystem::polarPoint(base, dist, 0.0);
    QVERIFY(std::abs(pt0.x() - 150.0) < 1e-4);
    QVERIFY(std::abs(pt0.y() - 100.0) < 1e-4);

    // 90 deg -> (100, 150)
    QPointF pt90 = CoordinateSystem::polarPoint(base, dist, 90.0);
    QVERIFY(std::abs(pt90.x() - 100.0) < 1e-4);
    QVERIFY(std::abs(pt90.y() - 150.0) < 1e-4);
}

void CadDimensionWorkflowTests::testOrthoSnapping() {
    QPointF base(50.0, 50.0);

    // Target mostly horizontal: (150, 60) -> snaps to Y=50
    QPointF orthoH = CoordinateSystem::snapToOrtho(base, QPointF(150.0, 60.0));
    QCOMPARE(orthoH, QPointF(150.0, 50.0));

    // Target mostly vertical: (60, 200) -> snaps to X=50
    QPointF orthoV = CoordinateSystem::snapToOrtho(base, QPointF(60.0, 200.0));
    QCOMPARE(orthoV, QPointF(50.0, 200.0));
}

void CadDimensionWorkflowTests::testPolarRaySnapping() {
    QPointF base(0.0, 0.0);
    bool snapped = false;

    // Point near 45 deg ray (e.g. (100, 102)) with increment 45 deg
    QPointF p = CoordinateSystem::snapToPolar(base, QPointF(100.0, 102.0), 45.0, 5.0, &snapped);
    QVERIFY(snapped);
    double ang = CoordinateSystem::angleDeg(base, p);
    QVERIFY(std::abs(ang - 45.0) < 1e-4);

    // Point far from any ray (e.g. at 20 deg) -> should NOT snap
    CoordinateSystem::snapToPolar(base, CoordinateSystem::polarPoint(base, 100.0, 20.0), 45.0, 3.0, &snapped);
    QVERIFY(!snapped);
}

// ============================================================================
// 4. Polygon Geometry Tests
// ============================================================================

void CadDimensionWorkflowTests::testPolygonGeometryAreaPerimeter() {
    // Regular hexagon with radius 100 mm
    PolygonEntity hex(QPointF(0.0, 0.0), 6, 100.0, 0.0);
    QCOMPARE(hex.sides(), 6);
    QCOMPARE(hex.radius(), 100.0);
    QVERIFY(hex.isClosed());

    // Theoretical area of regular hexagon: (3 * sqrt(3) / 2) * R^2 = ~25980.7621 sq mm
    double expectedArea = (3.0 * std::sqrt(3.0) / 2.0) * 100.0 * 100.0;
    QVERIFY(std::abs(hex.area() - expectedArea) < 1e-2);

    // Perimeter of regular hexagon: 6 * R = 600 mm
    QVERIFY(std::abs(hex.perimeter() - 600.0) < 1e-2);

    // Regular square with radius 100 mm (side = R * sqrt(2) = 141.42)
    PolygonEntity square(QPointF(0.0, 0.0), 4, 100.0, 0.0);
    double expectedSqArea = 0.5 * 4.0 * 100.0 * 100.0 * std::sin(2.0 * M_PI / 4.0); // 20000.0
    QVERIFY(std::abs(square.area() - expectedSqArea) < 1e-2);
}

void CadDimensionWorkflowTests::testPolygonSnapPoints() {
    PolygonEntity pentagon(QPointF(50.0, 50.0), 5, 80.0, 0.0);
    auto snaps = pentagon.snapPoints();

    // 1 center + 5 vertices + 5 midpoints = 11 snap points
    QCOMPARE(snaps.size(), static_cast<size_t>(11));
    QCOMPARE(snaps[0], QPointF(50.0, 50.0)); // Center
}

void CadDimensionWorkflowTests::testPolygonTransformations() {
    PolygonEntity hex(QPointF(0.0, 0.0), 6, 50.0, 0.0);
    hex.move(100.0, 200.0);
    QCOMPARE(hex.center(), QPointF(100.0, 200.0));

    // Rotate 30 degrees
    hex.rotate(QPointF(100.0, 200.0), M_PI / 6.0);
    QVERIFY(std::abs(hex.rotationDeg() - 30.0) < 1e-3);
}

void CadDimensionWorkflowTests::testPolygonSerialization() {
    PolygonEntity hex(QPointF(150.0, 250.0), 8, 120.0, 22.5);
    QJsonObject json = hex.toJson();

    PolygonEntity loaded;
    QVERIFY(loaded.fromJson(json));
    QCOMPARE(loaded.center(), QPointF(150.0, 250.0));
    QCOMPARE(loaded.sides(), 8);
    QCOMPARE(loaded.radius(), 120.0);
    QCOMPARE(loaded.rotationDeg(), 22.5);
}

// ============================================================================
// 5. Ellipse Geometry Tests
// ============================================================================

void CadDimensionWorkflowTests::testEllipseGeometryAreaPerimeter() {
    // Ellipse with rx = 100.0, ry = 50.0
    EllipseEntity ellipse(QPointF(0.0, 0.0), 100.0, 50.0, 0.0);
    QCOMPARE(ellipse.radiusX(), 100.0);
    QCOMPARE(ellipse.radiusY(), 50.0);
    QVERIFY(ellipse.isClosed());

    // Theoretical area: pi * a * b = pi * 100 * 50 = ~15707.96 sq mm
    double expectedArea = M_PI * 100.0 * 50.0;
    QVERIFY(std::abs(ellipse.area() - expectedArea) < 1e-4);

    // Ramanujan's perimeter: ~484.422 mm
    double h = std::pow(100.0 - 50.0, 2) / std::pow(100.0 + 50.0, 2);
    double expectedPerimeter = M_PI * (100.0 + 50.0) * (1.0 + (3.0 * h) / (10.0 + std::sqrt(4.0 - 3.0 * h)));
    QVERIFY(std::abs(ellipse.perimeter() - expectedPerimeter) < 1e-4);
}

void CadDimensionWorkflowTests::testEllipseQuadrantsAndSnapPoints() {
    EllipseEntity ellipse(QPointF(200.0, 150.0), 80.0, 40.0, 0.0);
    auto snaps = ellipse.snapPoints();

    // 1 center + 4 quadrants = 5 snap points
    QCOMPARE(snaps.size(), static_cast<size_t>(5));
    QCOMPARE(snaps[0], QPointF(200.0, 150.0)); // Center
    QCOMPARE(snaps[1], QPointF(280.0, 150.0)); // Right
    QCOMPARE(snaps[2], QPointF(120.0, 150.0)); // Left
    QCOMPARE(snaps[3], QPointF(200.0, 190.0)); // Top
    QCOMPARE(snaps[4], QPointF(200.0, 110.0)); // Bottom
}

void CadDimensionWorkflowTests::testEllipseTransformations() {
    EllipseEntity ellipse(QPointF(0.0, 0.0), 60.0, 30.0, 0.0);
    ellipse.move(50.0, -50.0);
    QCOMPARE(ellipse.center(), QPointF(50.0, -50.0));

    // Rotate 45 degrees
    ellipse.rotate(QPointF(50.0, -50.0), M_PI * 0.25);
    QVERIFY(std::abs(ellipse.rotationDeg() - 45.0) < 1e-3);
}

void CadDimensionWorkflowTests::testEllipseSerialization() {
    EllipseEntity ellipse(QPointF(100.0, 200.0), 90.0, 45.0, 30.0);
    QJsonObject json = ellipse.toJson();

    EllipseEntity loaded;
    QVERIFY(loaded.fromJson(json));
    QCOMPARE(loaded.center(), QPointF(100.0, 200.0));
    QCOMPARE(loaded.radiusX(), 90.0);
    QCOMPARE(loaded.radiusY(), 45.0);
    QCOMPARE(loaded.rotationDeg(), 30.0);
}

// ============================================================================
// 6. Associative Dimensions Tests
// ============================================================================

void CadDimensionWorkflowTests::testDimensionTypesMeasuredValues() {
    // Aligned
    DimensionEntity aligned(QPointF(0, 0), QPointF(30, 40), QPointF(15, 20), DimensionType::Aligned);
    QCOMPARE(aligned.measuredValue(), 50.0);

    // Horizontal
    DimensionEntity horiz(QPointF(10, 20), QPointF(110, 90), QPointF(60, 10), DimensionType::Horizontal);
    QCOMPARE(horiz.measuredValue(), 100.0);

    // Vertical
    DimensionEntity vert(QPointF(10, 20), QPointF(110, 90), QPointF(120, 55), DimensionType::Vertical);
    QCOMPARE(vert.measuredValue(), 70.0);

    // Radius
    DimensionEntity rad(QPointF(0, 0), QPointF(50, 0), QPointF(0, 0), DimensionType::Radius);
    QCOMPARE(rad.measuredValue(), 50.0);
    QCOMPARE(rad.dimensionText(), QStringLiteral("R50.00"));

    // Diameter
    DimensionEntity dia(QPointF(0, 0), QPointF(50, 0), QPointF(0, 0), DimensionType::Diameter);
    QCOMPARE(dia.measuredValue(), 100.0);
    QCOMPARE(dia.dimensionText(), QStringLiteral("Ø100.00"));
}

void CadDimensionWorkflowTests::testAssociativeDimensionTracksLine() {
    auto line = std::make_shared<LineEntity>(QPointF(0.0, 0.0), QPointF(100.0, 0.0));
    DimensionEntity dim(line->p1(), line->p2(), QPointF(50.0, 20.0), DimensionType::Aligned);
    dim.setTargetEntityId(line->id());

    QCOMPARE(dim.measuredValue(), 100.0);

    // Move line endpoint from 100 to 250
    line->setP2(QPointF(250.0, 0.0));
    QVERIFY(dim.updateFromTarget(line.get()));
    QCOMPARE(dim.p2(), QPointF(250.0, 0.0));
    QCOMPARE(dim.measuredValue(), 250.0);
}

void CadDimensionWorkflowTests::testAssociativeDimensionTracksCircle() {
    auto circle = std::make_shared<CircleEntity>(QPointF(100.0, 100.0), 40.0);
    DimensionEntity dim(circle->center(), QPointF(140.0, 100.0), circle->center(), DimensionType::Radius);
    dim.setTargetEntityId(circle->id());

    QCOMPARE(dim.measuredValue(), 40.0);

    // Change circle radius to 75.0
    circle->setRadius(75.0);
    QVERIFY(dim.updateFromTarget(circle.get()));
    QCOMPARE(dim.measuredValue(), 75.0);
    QCOMPARE(dim.dimensionText(), QStringLiteral("R75.00"));
}

void CadDimensionWorkflowTests::testDocumentSynchronizesAssociativeDimensions() {
    CadDocument doc;
    auto line = std::make_shared<LineEntity>(QPointF(0.0, 0.0), QPointF(120.0, 0.0));
    doc.addEntity(line);

    auto dim = std::make_shared<DimensionEntity>(line->p1(), line->p2(), QPointF(60.0, 15.0), DimensionType::Aligned);
    dim->setTargetEntityId(line->id());
    doc.addEntity(dim);

    QCOMPARE(dim->measuredValue(), 120.0);

    // Modify line geometry and notify document
    line->setP2(QPointF(300.0, 0.0));
    doc.entityModified(line);

    // Associative dimension automatically updated
    QCOMPARE(dim->p2(), QPointF(300.0, 0.0));
    QCOMPARE(dim->measuredValue(), 300.0);
}

// ============================================================================
// 7. Snapping Engine Tests
// ============================================================================

void CadDimensionWorkflowTests::testIntersectionSnap() {
    CadDocument doc;
    // Two lines intersecting at (40, 30), which is NOT a midpoint of either line
    auto l1 = std::make_shared<LineEntity>(QPointF(0.0, 30.0), QPointF(100.0, 30.0));
    auto l2 = std::make_shared<LineEntity>(QPointF(40.0, 0.0), QPointF(40.0, 100.0));
    doc.addEntity(l1);
    doc.addEntity(l2);

    SnapManager sm;
    sm.setSnapEnabled(true);
    sm.setModeEnabled(SnapType::Intersection, true);

    // Query near intersection (41, 29)
    SnapResult r = sm.findSnap(QPointF(41.0, 29.0), &doc, 10.0);
    QVERIFY(r.snapped);
    QCOMPARE(r.type, SnapType::Intersection);
    QCOMPARE(r.point, QPointF(40.0, 30.0));
}

void CadDimensionWorkflowTests::testPerpendicularSnap() {
    CadDocument doc;
    // Horizontal line y = 100
    auto line = std::make_shared<LineEntity>(QPointF(0.0, 100.0), QPointF(200.0, 100.0));
    doc.addEntity(line);

    SnapManager sm;
    sm.setSnapEnabled(true);
    sm.setModeEnabled(SnapType::Perpendicular, true);
    // Base point at (80, 0)
    sm.setBasePoint(QPointF(80.0, 0.0), true);

    // Near perpendicular point (80, 100)
    SnapResult r = sm.findSnap(QPointF(82.0, 98.0), &doc, 15.0);
    QVERIFY(r.snapped);
    QCOMPARE(r.type, SnapType::Perpendicular);
    QCOMPARE(r.point, QPointF(80.0, 100.0));
}

void CadDimensionWorkflowTests::testTangentSnap() {
    CadDocument doc;
    // Circle at origin with radius 50
    auto circle = std::make_shared<CircleEntity>(QPointF(0.0, 0.0), 50.0);
    doc.addEntity(circle);

    SnapManager sm;
    sm.setSnapEnabled(true);
    sm.setModeEnabled(SnapType::Tangent, true);
    // Base point outside circle at (100, 0)
    sm.setBasePoint(QPointF(100.0, 0.0), true);

    // Tangent angle is +- 60 degrees (cos = 50/100 = 0.5) -> (25, 43.30)
    QPointF expectedTangent(25.0, 50.0 * std::sin(M_PI / 3.0));
    SnapResult r = sm.findSnap(expectedTangent + QPointF(1.0, 1.0), &doc, 15.0);
    QVERIFY(r.snapped);
    QCOMPARE(r.type, SnapType::Tangent);
    QVERIFY(std::abs(r.point.x() - expectedTangent.x()) < 1e-2);
    QVERIFY(std::abs(r.point.y() - expectedTangent.y()) < 1e-2);
}

void CadDimensionWorkflowTests::testNearestSnap() {
    CadDocument doc;
    auto line = std::make_shared<LineEntity>(QPointF(0.0, 0.0), QPointF(200.0, 0.0));
    doc.addEntity(line);

    SnapManager sm;
    sm.setSnapEnabled(true);
    // Only Nearest enabled
    sm.setEnabledModes(static_cast<int>(SnapType::Nearest));

    // Query near middle of line at (75, 2)
    SnapResult r = sm.findSnap(QPointF(75.0, 2.0), &doc, 10.0);
    QVERIFY(r.snapped);
    QCOMPARE(r.type, SnapType::Nearest);
    QCOMPARE(r.point, QPointF(75.0, 0.0));
}

void CadDimensionWorkflowTests::testPolarTrackingSnap() {
    CadDocument doc;
    SnapManager sm;
    sm.setSnapEnabled(true);
    sm.setPolarEnabled(true);
    sm.setPolarAngleIncrement(45.0);
    sm.setBasePoint(QPointF(50.0, 50.0), true);

    // Mouse near 45-degree ray from base (50, 50): e.g. (150, 151)
    SnapResult r = sm.findSnap(QPointF(150.0, 151.0), &doc, 10.0);
    QVERIFY(r.snapped);
    QCOMPARE(r.type, SnapType::Polar);
    QVERIFY(r.hasPolarLine);
    QCOMPARE(r.polarStart, QPointF(50.0, 50.0));
    QVERIFY(std::abs(r.polarAngleDeg - 45.0) < 1e-3);
}

// ============================================================================
// 8. Full Persistence & Backward Compatibility Tests
// ============================================================================

void CadDimensionWorkflowTests::testFullWorkflowPersistenceRoundTrip() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString path = tempDir.filePath(QStringLiteral("full_workflow.gencad"));

    CadDocument doc;
    // Page setup: A3 Landscape
    PageSettings page(PageSize::A3, PageOrientation::Landscape);
    page.setDrawingLimits(10.0, 10.0, 410.0, 287.0);
    page.setEnforceLimits(true);
    doc.setPageSettings(page);

    // Unit settings: Centimeter, precision 3
    UnitSettings units(UnitType::Centimeter, 3);
    doc.setUnitSettings(units);

    // Add Polygon & Ellipse
    auto poly = std::make_shared<PolygonEntity>(QPointF(100.0, 100.0), 6, 40.0, 15.0);
    doc.addEntity(poly);

    auto ellipse = std::make_shared<EllipseEntity>(QPointF(250.0, 150.0), 60.0, 30.0, 25.0);
    doc.addEntity(ellipse);

    // Add Associative Dimension
    auto line = std::make_shared<LineEntity>(QPointF(0.0, 0.0), QPointF(150.0, 0.0));
    doc.addEntity(line);

    auto dim = std::make_shared<DimensionEntity>(line->p1(), line->p2(), QPointF(75.0, 20.0), DimensionType::Aligned);
    dim->setTargetEntityId(line->id());
    dim->setUnitSuffix(units.unitSuffix());
    doc.addEntity(dim);

    // Save
    QString error;
    QVERIFY(GencadFile::save(path, &doc, &error));

    // Load
    CadDocument loadedDoc;
    QVERIFY(GencadFile::load(path, &loadedDoc, &error));

    // Verify Page Settings
    QCOMPARE(loadedDoc.pageSettings().pageSize(), PageSize::A3);
    QCOMPARE(loadedDoc.pageSettings().orientation(), PageOrientation::Landscape);
    QCOMPARE(loadedDoc.pageSettings().minX(), 10.0);
    QCOMPARE(loadedDoc.pageSettings().maxX(), 410.0);
    QVERIFY(loadedDoc.pageSettings().enforceLimits());

    // Verify Unit Settings
    QCOMPARE(loadedDoc.unitSettings().type(), UnitType::Centimeter);
    QCOMPARE(loadedDoc.unitSettings().precision(), 3);
    QCOMPARE(loadedDoc.unitSettings().unitSuffix(), QStringLiteral("cm"));

    // Verify Entity count & types
    QCOMPARE(loadedDoc.entityCount(), static_cast<size_t>(4));

    // Verify Associative Dimension restored
    bool foundDim = false;
    for (const auto& ent : loadedDoc.entities()) {
        if (ent->type() == EntityType::Dimension) {
            foundDim = true;
            const auto* d = static_cast<const DimensionEntity*>(ent.get());
            QCOMPARE(d->targetEntityId(), line->id());
            QCOMPARE(d->measuredValue(), 150.0);
        }
    }
    QVERIFY(foundDim);
}

void CadDimensionWorkflowTests::testLegacyDrawingCompatibilityFallback() {
    // Old JSON format without pageSettings or unitSettings
    QJsonObject legacyRoot;
    legacyRoot["format"] = QStringLiteral("gencad");
    legacyRoot["version"] = QStringLiteral("2.0");

    QJsonArray entities;
    QJsonObject lineObj;
    lineObj["type"] = QStringLiteral("Line");
    lineObj["p1"] = QJsonObject{{"x", 0.0}, {"y", 0.0}};
    lineObj["p2"] = QJsonObject{{"x", 100.0}, {"y", 50.0}};
    entities.append(lineObj);
    legacyRoot["entities"] = entities;

    QJsonDocument jsonDoc(legacyRoot);
    QString jsonStr = QString::fromUtf8(jsonDoc.toJson());

    CadDocument doc;
    QString error;
    QVERIFY(GencadFile::fromJson(jsonStr, &doc, &error));

    // Default fallbacks are applied gracefully
    QCOMPARE(doc.pageSettings().pageSize(), PageSize::A4);
    QCOMPARE(doc.pageSettings().orientation(), PageOrientation::Landscape);
    QCOMPARE(doc.unitSettings().type(), UnitType::Millimeter);
    QCOMPARE(doc.unitSettings().precision(), 2);
    QCOMPARE(doc.entityCount(), static_cast<size_t>(1));
}

// ============================================================================
// 9. Section 29 Acceptance Tests
// ============================================================================

void CadDimensionWorkflowTests::testAcceptance1_HorizontalLine() {
    CadDocument doc;
    CadScene scene(&doc);
    CadView view(&scene);
    SnapManager snapMgr;
    view.setSnapManager(&snapMgr);
    view.setOrthoEnabled(true);
    QVERIFY(view.isOrthoEnabled());

    LineTool lineTool(&doc, &view, nullptr);
    view.setTool(&lineTool);

    // Start = 100, 100
    QMouseEvent press1(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    lineTool.mousePress(&press1, QPointF(100.0, 100.0));
    QVERIFY(snapMgr.hasBasePoint());
    QCOMPARE(snapMgr.basePoint(), QPointF(100.0, 100.0));

    // Move mouse diagonally: raw dx = 200, dy = 50 -> dominant horizontal dx >= dy
    QPointF mouseRaw(300.0, 150.0);
    QPointF snapped = view.getSnappedPosition(mouseRaw);
    // Expected: Horizontal line, Angle = 0° or 180°, no accidental diagonal
    QCOMPARE(snapped.y(), 100.0);
    QCOMPARE(snapped.x(), 300.0);
    double angle = CoordinateSystem::angleDeg(snapMgr.basePoint(), snapped);
    QCOMPARE(angle, 0.0);
}

void CadDimensionWorkflowTests::testAcceptance2_VerticalLine() {
    CadDocument doc;
    CadScene scene(&doc);
    CadView view(&scene);
    SnapManager snapMgr;
    view.setSnapManager(&snapMgr);
    view.setOrthoEnabled(true);

    LineTool lineTool(&doc, &view, nullptr);
    view.setTool(&lineTool);

    // Start = 100, 100
    QMouseEvent press1(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    lineTool.mousePress(&press1, QPointF(100.0, 100.0));

    // Move mouse vertically: raw dx = 20, dy = 300 -> dominant vertical dy > dx
    QPointF mouseRaw(120.0, 400.0);
    QPointF snapped = view.getSnappedPosition(mouseRaw);
    // Expected: Vertical line, Angle = 90° or 270°
    QCOMPARE(snapped.x(), 100.0);
    QCOMPARE(snapped.y(), 400.0);
    double angle = CoordinateSystem::angleDeg(snapMgr.basePoint(), snapped);
    QCOMPARE(angle, 90.0);
}

void CadDimensionWorkflowTests::testAcceptance3_F8() {
    CadDocument doc;
    CadScene scene(&doc);
    CadView view(&scene);
    view.setOrthoEnabled(false);
    QVERIFY(!view.isOrthoEnabled());

    // Press F8 -> ORTHO ON
    QKeyEvent f8Press(QEvent::KeyPress, Qt::Key_F8, Qt::NoModifier);
    QApplication::sendEvent(&view, &f8Press);
    QVERIFY(view.isOrthoEnabled());

    // Press F8 again -> ORTHO OFF
    QApplication::sendEvent(&view, &f8Press);
    QVERIFY(!view.isOrthoEnabled());
}

void CadDimensionWorkflowTests::testAcceptance4_OrthoOff() {
    CadDocument doc;
    CadScene scene(&doc);
    CadView view(&scene);
    SnapManager snapMgr;
    view.setSnapManager(&snapMgr);
    view.setOrthoEnabled(false);

    LineTool lineTool(&doc, &view, nullptr);
    view.setTool(&lineTool);

    // Start = 100, 100
    QMouseEvent press1(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    lineTool.mousePress(&press1, QPointF(100.0, 100.0));

    // Diagonal drawing is allowed
    QPointF mouseRaw(250.0, 350.0);
    QPointF snapped = view.getSnappedPosition(mouseRaw);
    QCOMPARE(snapped.x(), 250.0);
    QCOMPARE(snapped.y(), 350.0);
    double angle = CoordinateSystem::angleDeg(snapMgr.basePoint(), snapped);
    QVERIFY(std::abs(angle - 59.036) < 1.0);
}

void CadDimensionWorkflowTests::testAcceptance5_ExactHorizontalDimension() {
    CadDocument doc;
    CadScene scene(&doc);
    CadView view(&scene);
    SnapManager snapMgr;
    view.setSnapManager(&snapMgr);
    view.setOrthoEnabled(true);

    LineTool lineTool(&doc, &view, nullptr);
    view.setTool(&lineTool);

    // Start = 100, 100
    QMouseEvent press1(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    lineTool.mousePress(&press1, QPointF(100.0, 100.0));

    // Direction = Horizontal, Length = 1000 mm
    lineTool.commitExact(1000.0, 0.0);

    // Expected: Exact 1000 mm horizontal line + associative dimension
    QCOMPARE(doc.entityCount(), static_cast<size_t>(2));
    auto line = std::dynamic_pointer_cast<LineEntity>(doc.entities().front());
    QVERIFY(line != nullptr);
    QCOMPARE(line->p1(), QPointF(100.0, 100.0));
    QCOMPARE(line->p2(), QPointF(1100.0, 100.0));
    QCOMPARE(line->length(), 1000.0);

    auto dim = std::dynamic_pointer_cast<DimensionEntity>(doc.entities().back());
    QVERIFY(dim != nullptr);
    QCOMPARE(dim->measuredValue(), 1000.0);
    QCOMPARE(dim->dimensionText(), QStringLiteral("1000.00 mm"));
}

void CadDimensionWorkflowTests::testAcceptance6_ExactVerticalDimension() {
    CadDocument doc;
    CadScene scene(&doc);
    CadView view(&scene);
    SnapManager snapMgr;
    view.setSnapManager(&snapMgr);
    view.setOrthoEnabled(true);

    LineTool lineTool(&doc, &view, nullptr);
    view.setTool(&lineTool);

    // Start = 100, 100
    QMouseEvent press1(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    lineTool.mousePress(&press1, QPointF(100.0, 100.0));

    // Direction = Vertical, Length = 1000 mm
    lineTool.commitExact(1000.0, 90.0);

    // Expected: Exact 1000 mm vertical line + associative dimension
    QCOMPARE(doc.entityCount(), static_cast<size_t>(2));
    auto line = std::dynamic_pointer_cast<LineEntity>(doc.entities().front());
    QVERIFY(line != nullptr);
    QCOMPARE(line->p1(), QPointF(100.0, 100.0));
    QCOMPARE(line->p2(), QPointF(100.0, 1100.0));
    QCOMPARE(line->length(), 1000.0);

    auto dim = std::dynamic_pointer_cast<DimensionEntity>(doc.entities().back());
    QVERIFY(dim != nullptr);
    QCOMPARE(dim->measuredValue(), 1000.0);
    QCOMPARE(dim->dimensionText(), QStringLiteral("1000.00 mm"));
}

void CadDimensionWorkflowTests::testAcceptance7_Rectangle() {
    CadDocument doc;
    CadScene scene(&doc);
    CadView view(&scene);
    SnapManager snapMgr;
    view.setSnapManager(&snapMgr);
    view.setOrthoEnabled(true);

    RectangleTool rectTool(&doc, &view, nullptr);
    view.setTool(&rectTool);

    // Start = 0, 0
    QMouseEvent press1(QEvent::MouseButtonPress, QPointF(0, 0), QPointF(0, 0), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    rectTool.mousePress(&press1, QPointF(0.0, 0.0));

    // Length = 1000 mm, Width = 500 mm
    rectTool.commitExact(1000.0, 500.0);

    // Expected: Exact 1000 x 500 mm rectangle + 2 associative dimensions (Length & Width)
    QCOMPARE(doc.entityCount(), static_cast<size_t>(3));
    auto rect = std::dynamic_pointer_cast<RectangleEntity>(doc.entities().front());
    QVERIFY(rect != nullptr);
    QCOMPARE(rect->width(), 1000.0);
    QCOMPARE(rect->height(), 500.0);
    QCOMPARE(rect->area(), 500000.0);
    QCOMPARE(rect->perimeter(), 3000.0);

    size_t dimCount = 0;
    for (const auto& ent : doc.entities()) {
        if (ent->type() == EntityType::Dimension) {
            auto d = std::dynamic_pointer_cast<DimensionEntity>(ent);
            QVERIFY(d != nullptr);
            QCOMPARE(d->targetEntityId(), rect->id());
            dimCount++;
        }
    }
    QCOMPARE(dimCount, static_cast<size_t>(2));
}

void CadDimensionWorkflowTests::testAcceptance8_Polyline() {
    CadDocument doc;
    CadScene scene(&doc);
    CadView view(&scene);
    SnapManager snapMgr;
    view.setSnapManager(&snapMgr);
    view.setOrthoEnabled(true);

    PolylineTool polyTool(&doc, &view, nullptr);
    view.setTool(&polyTool);

    // Start = 0, 0
    QMouseEvent p1(QEvent::MouseButtonPress, QPointF(0, 0), QPointF(0, 0), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    polyTool.mousePress(&p1, QPointF(0.0, 0.0));

    // 500 mm horizontal
    polyTool.mouseMove(nullptr, QPointF(100.0, 0.0));
    polyTool.commitExact(500.0, 0.0);

    // 300 mm vertical
    polyTool.mouseMove(nullptr, QPointF(500.0, 100.0));
    polyTool.commitExact(300.0, 90.0);

    // 500 mm horizontal
    polyTool.mouseMove(nullptr, QPointF(600.0, 300.0));
    polyTool.commitExact(500.0, 0.0);

    // Finish polyline with Enter
    QKeyEvent enterKey(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    polyTool.keyPress(&enterKey);

    // Expected: 1 Polyline + 3 associative segment dimensions
    QCOMPARE(doc.entityCount(), static_cast<size_t>(4));
    auto poly = std::dynamic_pointer_cast<PolylineEntity>(doc.entities().front());
    QVERIFY(poly != nullptr);
    QCOMPARE(poly->pointCount(), static_cast<size_t>(4));
    QCOMPARE(poly->points()[0], QPointF(0.0, 0.0));
    QCOMPARE(poly->points()[1], QPointF(500.0, 0.0));
    QCOMPARE(poly->points()[2], QPointF(500.0, 300.0));
    QCOMPARE(poly->points()[3], QPointF(1000.0, 300.0));
    QCOMPARE(poly->totalLength(), 1300.0);
}

void CadDimensionWorkflowTests::testAcceptance9_GridSnapPlusOrtho() {
    CadDocument doc;
    CadScene scene(&doc);
    CadView view(&scene);
    SnapManager snapMgr;
    view.setSnapManager(&snapMgr);
    view.setOrthoEnabled(true);
    snapMgr.setGridSnapEnabled(true);
    snapMgr.setGridSize(20.0);
    snapMgr.setSnapEnabled(false);

    LineTool lineTool(&doc, &view, nullptr);
    view.setTool(&lineTool);

    // Start = 100, 100
    QMouseEvent press1(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    lineTool.mousePress(&press1, QPointF(100.0, 100.0));

    // Cursor at (212, 130): dx=112 > dy=30 -> dominant horizontal (212, 100)
    // Grid Snap snaps x=212 to nearest multiple of 20 (220), y stays 100
    QPointF snapped = view.getSnappedPosition(QPointF(212.0, 130.0));
    QCOMPARE(snapped, QPointF(220.0, 100.0));
}

void CadDimensionWorkflowTests::testAcceptance10_ObjectSnapPlusOrtho() {
    CadDocument doc;
    CadScene scene(&doc);
    CadView view(&scene);
    SnapManager snapMgr;
    view.setSnapManager(&snapMgr);
    view.setOrthoEnabled(true);
    snapMgr.setSnapEnabled(true);
    snapMgr.setModeEnabled(SnapType::Endpoint, true);
    snapMgr.setGridSnapEnabled(false);

    // Existing line with endpoint at (200, 140)
    auto line = std::make_shared<LineEntity>(QPointF(200.0, 140.0), QPointF(300.0, 140.0));
    doc.addEntity(line);

    LineTool lineTool(&doc, &view, nullptr);
    view.setTool(&lineTool);

    // Start = 100, 100
    QMouseEvent press1(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    lineTool.mousePress(&press1, QPointF(100.0, 100.0));

    // Cursor near (202, 138) snaps to (200, 140), then ORTHO projects onto horizontal axis (200, 100)
    QPointF snapped = view.getSnappedPosition(QPointF(202.0, 138.0));
    QCOMPARE(snapped, QPointF(200.0, 100.0));
}

void CadDimensionWorkflowTests::testAcceptance11_DimensionEditing() {
    CadDocument doc;
    // Create 1000 x 500 mm
    auto rect = std::make_shared<RectangleEntity>(QPointF(0.0, 0.0), 1000.0, 500.0);
    doc.addEntity(rect);

    auto dimLen = std::make_shared<DimensionEntity>(QPointF(0, 0), QPointF(1000, 0), QPointF(500, -15), DimensionType::Horizontal);
    dimLen->setTargetEntityId(rect->id());
    dimLen->setTargetFeatureIndex(0);
    dimLen->setUnitSuffix(doc.unitSettings().unitSuffix());
    doc.addEntity(dimLen);

    auto dimWid = std::make_shared<DimensionEntity>(QPointF(1000, 0), QPointF(1000, 500), QPointF(1015, 250), DimensionType::Vertical);
    dimWid->setTargetEntityId(rect->id());
    dimWid->setTargetFeatureIndex(1);
    dimWid->setUnitSuffix(doc.unitSettings().unitSuffix());
    doc.addEntity(dimWid);

    QCOMPARE(rect->width(), 1000.0);
    QCOMPARE(rect->height(), 500.0);
    QCOMPARE(dimLen->measuredValue(), 1000.0);
    QCOMPARE(dimWid->measuredValue(), 500.0);

    // Edit: 1500 x 750 mm
    rect->setWidth(1500.0);
    rect->setHeight(750.0);
    doc.entityModified(rect);

    // Expected: Actual geometry = 1500 x 750 mm, associative dimensions updated
    QCOMPARE(rect->width(), 1500.0);
    QCOMPARE(rect->height(), 750.0);
    QCOMPARE(dimLen->measuredValue(), 1500.0);
    QCOMPARE(dimWid->measuredValue(), 750.0);
    QCOMPARE(dimLen->dimensionText(), QStringLiteral("1500.00 mm"));
    QCOMPARE(dimWid->dimensionText(), QStringLiteral("750.00 mm"));
}

void CadDimensionWorkflowTests::testAcceptance12_SaveLoad() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString filePath = tempDir.filePath(QStringLiteral("acceptance_ortho.gencad"));

    CadDocument doc;
    doc.unitSettings().setType(UnitType::Millimeter);
    doc.unitSettings().setPrecision(2);

    // Create dimensioned ORTHO geometry
    auto line = std::make_shared<LineEntity>(QPointF(100.0, 100.0), QPointF(1100.0, 100.0));
    doc.addEntity(line);

    auto dimLine = std::make_shared<DimensionEntity>(line->p1(), line->p2(), QPointF(600.0, 80.0), DimensionType::Horizontal);
    dimLine->setTargetEntityId(line->id());
    dimLine->setUnitSuffix(doc.unitSettings().unitSuffix());
    doc.addEntity(dimLine);

    auto rect = std::make_shared<RectangleEntity>(QPointF(200.0, 200.0), 1000.0, 500.0);
    doc.addEntity(rect);

    auto dimRect = std::make_shared<DimensionEntity>(QPointF(200, 200), QPointF(1200, 200), QPointF(700, 180), DimensionType::Horizontal);
    dimRect->setTargetEntityId(rect->id());
    dimRect->setUnitSuffix(doc.unitSettings().unitSuffix());
    doc.addEntity(dimRect);

    // Save
    QString error;
    QVERIFY(GencadFile::save(filePath, &doc, &error));
    QVERIFY(error.isEmpty());

    // Close / Reopen in clean document
    CadDocument loadedDoc;
    QVERIFY(GencadFile::load(filePath, &loadedDoc, &error));
    QVERIFY(error.isEmpty());

    // Geometry preserved
    QCOMPARE(loadedDoc.entityCount(), static_cast<size_t>(4));
    auto loadedLine = std::dynamic_pointer_cast<LineEntity>(loadedDoc.findEntity(line->id()));
    QVERIFY(loadedLine != nullptr);
    QCOMPARE(loadedLine->length(), 1000.0);

    auto loadedRect = std::dynamic_pointer_cast<RectangleEntity>(loadedDoc.findEntity(rect->id()));
    QVERIFY(loadedRect != nullptr);
    QCOMPARE(loadedRect->width(), 1000.0);
    QCOMPARE(loadedRect->height(), 500.0);

    // Dimensions & annotations preserved
    auto loadedDimLine = std::dynamic_pointer_cast<DimensionEntity>(loadedDoc.findEntity(dimLine->id()));
    QVERIFY(loadedDimLine != nullptr);
    QCOMPARE(loadedDimLine->measuredValue(), 1000.0);
    QCOMPARE(loadedDimLine->dimensionText(), QStringLiteral("1000.00 mm"));
    QCOMPARE(loadedDimLine->targetEntityId(), line->id());

    // Units preserved
    QCOMPARE(loadedDoc.unitSettings().type(), UnitType::Millimeter);
    QCOMPARE(loadedDoc.unitSettings().precision(), 2);
}

#include "CadDimensionWorkflowTests.moc"

QObject* createCadDimensionWorkflowTests() {
    return new CadDimensionWorkflowTests();
}
