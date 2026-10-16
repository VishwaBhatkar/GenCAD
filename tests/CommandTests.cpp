#include <QTest>
#include "document/CadDocument.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "commands/CreateEntityCommand.h"
#include "commands/DeleteEntityCommand.h"
#include "commands/MoveEntityCommand.h"

class CommandTests : public QObject {
    Q_OBJECT

private slots:
    void testCreateEntityCommand();
    void testDeleteEntityCommand();
    void testMoveEntityCommand();
};

void CommandTests::testCreateEntityCommand() {
    CadDocument doc;
    QCOMPARE(doc.entityCount(), 0ULL);

    auto line = std::make_shared<LineEntity>(QPointF(0, 0), QPointF(50, 50));
    QUuid lineId = line->id();

    // Push command
    doc.undoStack()->push(new CreateEntityCommand(&doc, line));
    QCOMPARE(doc.entityCount(), 1ULL);
    QVERIFY(doc.findEntity(lineId) != nullptr);

    // Undo
    doc.undoStack()->undo();
    QCOMPARE(doc.entityCount(), 0ULL);
    QVERIFY(doc.findEntity(lineId) == nullptr);

    // Redo
    doc.undoStack()->redo();
    QCOMPARE(doc.entityCount(), 1ULL);
    QVERIFY(doc.findEntity(lineId) != nullptr);
}

void CommandTests::testDeleteEntityCommand() {
    CadDocument doc;
    auto circle = std::make_shared<CircleEntity>(QPointF(100, 100), 20);
    QUuid circleId = circle->id();
    doc.addEntity(circle);
    QCOMPARE(doc.entityCount(), 1ULL);

    // Push delete command
    doc.undoStack()->push(new DeleteEntityCommand(&doc, circle));
    QCOMPARE(doc.entityCount(), 0ULL);

    // Undo delete -> restores entity
    doc.undoStack()->undo();
    QCOMPARE(doc.entityCount(), 1ULL);
    QVERIFY(doc.findEntity(circleId) != nullptr);

    // Redo delete -> removes entity again
    doc.undoStack()->redo();
    QCOMPARE(doc.entityCount(), 0ULL);
}

void CommandTests::testMoveEntityCommand() {
    CadDocument doc;
    auto line = std::make_shared<LineEntity>(QPointF(10, 10), QPointF(20, 20));
    QUuid lineId = line->id();
    doc.addEntity(line);

    // Push move command
    doc.undoStack()->push(new MoveEntityCommand(&doc, {lineId}, 15.0, 25.0));
    QCOMPARE(line->p1(), QPointF(25.0, 35.0));
    QCOMPARE(line->p2(), QPointF(35.0, 45.0));

    // Undo move
    doc.undoStack()->undo();
    QCOMPARE(line->p1(), QPointF(10.0, 10.0));
    QCOMPARE(line->p2(), QPointF(20.0, 20.0));

    // Redo move
    doc.undoStack()->redo();
    QCOMPARE(line->p1(), QPointF(25.0, 35.0));
    QCOMPARE(line->p2(), QPointF(35.0, 45.0));
}

#include "CommandTests.moc"

QObject* createCommandTests() {
    return new CommandTests();
}
