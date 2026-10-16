#include <QTest>
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"

class GeometryTests : public QObject {
    Q_OBJECT

private slots:
    void testLineCoordinates();
    void testLineLength();
    void testLineHitTest();
    void testLineMove();

    void testCircleCenterRadius();
    void testCircleAreaCircumference();
    void testCircleHitTest();
    void testCircleMove();

    void testRectangleCoordinates();
    void testRectangleAreaPerimeter();
    void testRectangleHitTest();
    void testRectangleMove();
};

void GeometryTests::testLineCoordinates() {
    QPointF p1(10.0, 20.0);
    QPointF p2(100.0, 150.0);
    LineEntity line(p1, p2);

    QCOMPARE(line.p1(), p1);
    QCOMPARE(line.p2(), p2);
    QCOMPARE(line.type(), EntityType::Line);
    QVERIFY(!line.id().isNull());
}

void GeometryTests::testLineLength() {
    LineEntity line(QPointF(0.0, 0.0), QPointF(3.0, 4.0));
    QCOMPARE(line.length(), 5.0);
}

void GeometryTests::testLineHitTest() {
    LineEntity line(QPointF(0.0, 0.0), QPointF(10.0, 0.0));
    QVERIFY(line.hitTest(QPointF(5.0, 0.5), 1.0));
    QVERIFY(!line.hitTest(QPointF(5.0, 2.0), 1.0));
    QVERIFY(line.hitTest(QPointF(0.0, 0.0), 0.1));
    QVERIFY(line.hitTest(QPointF(10.0, 0.0), 0.1));
}

void GeometryTests::testLineMove() {
    LineEntity line(QPointF(10.0, 10.0), QPointF(20.0, 30.0));
    line.move(5.0, -2.0);
    QCOMPARE(line.p1(), QPointF(15.0, 8.0));
    QCOMPARE(line.p2(), QPointF(25.0, 28.0));
}

void GeometryTests::testCircleCenterRadius() {
    QPointF center(50.0, 60.0);
    double radius = 25.0;
    CircleEntity circle(center, radius);

    QCOMPARE(circle.center(), center);
    QCOMPARE(circle.radius(), radius);
    QCOMPARE(circle.type(), EntityType::Circle);
    QVERIFY(!circle.id().isNull());
}

void GeometryTests::testCircleAreaCircumference() {
    CircleEntity circle(QPointF(0.0, 0.0), 10.0);
    QVERIFY(std::abs(circle.area() - (3.141592653589793 * 100.0)) < 1e-4);
    QVERIFY(std::abs(circle.circumference() - (2.0 * 3.141592653589793 * 10.0)) < 1e-4);
}

void GeometryTests::testCircleHitTest() {
    CircleEntity circle(QPointF(0.0, 0.0), 10.0);
    // Point on circumference
    QVERIFY(circle.hitTest(QPointF(10.0, 0.0), 0.5));
    QVERIFY(circle.hitTest(QPointF(0.0, 10.2), 0.5));
    // Point far from boundary
    QVERIFY(!circle.hitTest(QPointF(0.0, 0.0), 0.5));
    QVERIFY(!circle.hitTest(QPointF(20.0, 0.0), 0.5));
}

void GeometryTests::testCircleMove() {
    CircleEntity circle(QPointF(10.0, 20.0), 15.0);
    circle.move(-5.0, 10.0);
    QCOMPARE(circle.center(), QPointF(5.0, 30.0));
    QCOMPARE(circle.radius(), 15.0);
}

void GeometryTests::testRectangleCoordinates() {
    QPointF topLeft(15.0, 25.0);
    RectangleEntity rect(topLeft, 80.0, 40.0);

    QCOMPARE(rect.topLeft(), topLeft);
    QCOMPARE(rect.width(), 80.0);
    QCOMPARE(rect.height(), 40.0);
    QCOMPARE(rect.type(), EntityType::Rectangle);
}

void GeometryTests::testRectangleAreaPerimeter() {
    RectangleEntity rect(QPointF(0.0, 0.0), 10.0, 20.0);
    QCOMPARE(rect.area(), 200.0);
    QCOMPARE(rect.perimeter(), 60.0);
}

void GeometryTests::testRectangleHitTest() {
    RectangleEntity rect(QPointF(0.0, 0.0), 100.0, 50.0);
    // Near top edge
    QVERIFY(rect.hitTest(QPointF(50.0, 0.5), 1.0));
    // Near bottom edge
    QVERIFY(rect.hitTest(QPointF(50.0, 49.5), 1.0));
    // Inside center (far from edges)
    QVERIFY(!rect.hitTest(QPointF(50.0, 25.0), 1.0));
}

void GeometryTests::testRectangleMove() {
    RectangleEntity rect(QPointF(10.0, 10.0), 50.0, 30.0);
    rect.move(20.0, 15.0);
    QCOMPARE(rect.topLeft(), QPointF(30.0, 25.0));
    QCOMPARE(rect.width(), 50.0);
    QCOMPARE(rect.height(), 30.0);
}

#include "GeometryTests.moc"

QObject* createGeometryTests() {
    return new GeometryTests();
}
