#include <QTest>
#include "document/CadDocument.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"

class DocumentTests : public QObject {
    Q_OBJECT

private slots:
    void testAddAndFindEntity();
    void testRemoveEntity();
    void testClear();
    void testModifiedState();
    void testSelection();
    void testBoundingBox();
};

void DocumentTests::testAddAndFindEntity() {
    CadDocument doc;
    QCOMPARE(doc.entityCount(), 0ULL);

    auto line = std::make_shared<LineEntity>(QPointF(0, 0), QPointF(10, 10));
    QUuid lineId = line->id();

    QVERIFY(doc.addEntity(line));
    QCOMPARE(doc.entityCount(), 1ULL);

    auto found = doc.findEntity(lineId);
    QVERIFY(found != nullptr);
    QCOMPARE(found->id(), lineId);
    QCOMPARE(found->type(), EntityType::Line);

    // Duplicate add should fail
    QVERIFY(!doc.addEntity(line));
    QCOMPARE(doc.entityCount(), 1ULL);
}

void DocumentTests::testRemoveEntity() {
    CadDocument doc;
    auto circle = std::make_shared<CircleEntity>(QPointF(20, 20), 10);
    QUuid circleId = circle->id();

    doc.addEntity(circle);
    QCOMPARE(doc.entityCount(), 1ULL);

    QVERIFY(doc.removeEntity(circleId));
    QCOMPARE(doc.entityCount(), 0ULL);
    QVERIFY(doc.findEntity(circleId) == nullptr);

    // Non-existent remove should return false
    QVERIFY(!doc.removeEntity(circleId));
}

void DocumentTests::testClear() {
    CadDocument doc;
    doc.addEntity(std::make_shared<LineEntity>(QPointF(0, 0), QPointF(10, 10)));
    doc.addEntity(std::make_shared<CircleEntity>(QPointF(20, 20), 5));
    QCOMPARE(doc.entityCount(), 2ULL);

    doc.clear();
    QCOMPARE(doc.entityCount(), 0ULL);
    QVERIFY(!doc.isModified());
}

void DocumentTests::testModifiedState() {
    CadDocument doc;
    QVERIFY(!doc.isModified());

    doc.setModified(true);
    QVERIFY(doc.isModified());

    doc.setModified(false);
    QVERIFY(!doc.isModified());
}

void DocumentTests::testSelection() {
    CadDocument doc;
    auto line = std::make_shared<LineEntity>(QPointF(0, 0), QPointF(10, 10));
    auto circle = std::make_shared<CircleEntity>(QPointF(20, 20), 5);
    doc.addEntity(line);
    doc.addEntity(circle);

    QCOMPARE(doc.selectionCount(), 0ULL);

    doc.select(line->id(), false);
    QCOMPARE(doc.selectionCount(), 1ULL);
    QVERIFY(doc.isSelected(line->id()));
    QVERIFY(!doc.isSelected(circle->id()));

    // Multi-select
    doc.select(circle->id(), true);
    QCOMPARE(doc.selectionCount(), 2ULL);

    // Deselect
    doc.deselect(line->id());
    QCOMPARE(doc.selectionCount(), 1ULL);
    QVERIFY(!doc.isSelected(line->id()));
    QVERIFY(doc.isSelected(circle->id()));

    // Clear
    doc.clearSelection();
    QCOMPARE(doc.selectionCount(), 0ULL);
}

void DocumentTests::testBoundingBox() {
    CadDocument doc;
    auto line = std::make_shared<LineEntity>(QPointF(-10, -10), QPointF(10, 10));
    doc.addEntity(line);

    QRectF box = doc.boundingBox();
    QCOMPARE(box.left(), -10.0);
    QCOMPARE(box.top(), -10.0);
    QCOMPARE(box.right(), 10.0);
    QCOMPARE(box.bottom(), 10.0);
}

#include "DocumentTests.moc"

QObject* createDocumentTests() {
    return new DocumentTests();
}
