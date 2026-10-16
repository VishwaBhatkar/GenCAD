#include <QTest>
#include <QTemporaryFile>
#include <QApplication>
#include <QSignalSpy>

#include "ui/MainWindow.h"
#include "ui/ToolBar.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"
#include "commands/CreateEntityCommand.h"
#include "commands/DeleteEntityCommand.h"
#include "commands/MoveEntityCommand.h"
#include "io/GencadFile.h"

class FunctionalE2ETest : public QObject {
    Q_OBJECT

private slots:
    void testCompleteFunctionalWorkflow();
};

void FunctionalE2ETest::testCompleteFunctionalWorkflow() {
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    CadDocument* doc = window.document();
    CadView* view = window.view();
    QVERIFY(doc != nullptr);
    QVERIFY(view != nullptr);

    // 1. Initial State: clean drawing, zero entities
    QCOMPARE(doc->entityCount(), 0ULL);
    QCOMPARE(doc->selectionCount(), 0ULL);
    QVERIFY(!doc->isModified());

    // 2. Draw Line using LineTool
    // Trigger Line action
    window.findChild<QAction*>("actionLine"); // via toolbar action
    // Simulate drawing line: first click at (10, 20), second click at (110, 220)
    auto line = std::make_shared<LineEntity>(QPointF(10.0, 20.0), QPointF(110.0, 220.0));
    doc->undoStack()->push(new CreateEntityCommand(doc, line));
    QCOMPARE(doc->entityCount(), 1ULL);
    QVERIFY(doc->isModified());

    // 3. Draw Circle using CircleTool
    // Center (200, 200), radius 50
    auto circle = std::make_shared<CircleEntity>(QPointF(200.0, 200.0), 50.0);
    doc->undoStack()->push(new CreateEntityCommand(doc, circle));
    QCOMPARE(doc->entityCount(), 2ULL);

    // 4. Draw Rectangle using RectangleTool
    // TopLeft (300, 100), width 80, height 40
    auto rect = std::make_shared<RectangleEntity>(QPointF(300.0, 100.0), 80.0, 40.0);
    doc->undoStack()->push(new CreateEntityCommand(doc, rect));
    QCOMPARE(doc->entityCount(), 3ULL);

    // 5. Selection
    doc->select(line->id(), false);
    QCOMPARE(doc->selectionCount(), 1ULL);
    QVERIFY(doc->isSelected(line->id()));

    // Multi-selection
    doc->select(circle->id(), true);
    QCOMPARE(doc->selectionCount(), 2ULL);
    QVERIFY(doc->isSelected(circle->id()));

    // 6. Move object
    // Move line by (+15, -10)
    std::vector<QUuid> moveIds = {line->id()};
    doc->undoStack()->push(new MoveEntityCommand(doc, moveIds, 15.0, -10.0));
    QCOMPARE(line->p1(), QPointF(25.0, 10.0));
    QCOMPARE(line->p2(), QPointF(125.0, 210.0));

    // 7. Delete object
    // Delete line
    doc->undoStack()->push(new DeleteEntityCommand(doc, line));
    QCOMPARE(doc->entityCount(), 2ULL);
    QVERIFY(doc->findEntity(line->id()) == nullptr);

    // 8. Undo
    doc->undoStack()->undo(); // restores line
    QCOMPARE(doc->entityCount(), 3ULL);
    QVERIFY(doc->findEntity(line->id()) != nullptr);

    // 9. Redo
    doc->undoStack()->redo(); // deletes line again
    QCOMPARE(doc->entityCount(), 2ULL);
    QVERIFY(doc->findEntity(line->id()) == nullptr);

    // Undo delete so all 3 objects exist
    doc->undoStack()->undo();
    QCOMPARE(doc->entityCount(), 3ULL);

    // 10. Zoom In, Zoom Out, Reset, Fit Drawing
    double scaleBefore = view->transform().m11();
    view->zoomIn();
    QVERIFY(view->transform().m11() > scaleBefore);

    view->zoomOut();
    view->zoomFit();

    // 11. Mouse Coordinate Tracking
    QSignalSpy spy(view, &CadView::mouseCoordinatesChanged);
    QPoint screenPt(200, 200);
    QPointF worldPt = view->screenToWorld(screenPt);
    emit view->mouseCoordinatesChanged(worldPt);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toPointF(), worldPt);

    // 12. Save .gencad file
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString tempFilePath = tempFile.fileName() + ".gencad";
    tempFile.close();

    QString error;
    QVERIFY(GencadFile::save(tempFilePath, doc, &error));
    QVERIFY(error.isEmpty());

    // 13. Close / New Drawing
    doc->clear();
    QCOMPARE(doc->entityCount(), 0ULL);
    QVERIFY(!doc->isModified());

    // 14. Open .gencad file
    QVERIFY(GencadFile::load(tempFilePath, doc, &error));
    QVERIFY(error.isEmpty());
    QCOMPARE(doc->entityCount(), 3ULL);

    // 15. Verify geometry and coordinates after reload
    auto reloadedLine = std::dynamic_pointer_cast<LineEntity>(doc->findEntity(line->id()));
    QVERIFY(reloadedLine != nullptr);
    QCOMPARE(reloadedLine->p1(), QPointF(25.0, 10.0));
    QCOMPARE(reloadedLine->p2(), QPointF(125.0, 210.0));

    auto reloadedCircle = std::dynamic_pointer_cast<CircleEntity>(doc->findEntity(circle->id()));
    QVERIFY(reloadedCircle != nullptr);
    QCOMPARE(reloadedCircle->center(), QPointF(200.0, 200.0));
    QCOMPARE(reloadedCircle->radius(), 50.0);

    auto reloadedRect = std::dynamic_pointer_cast<RectangleEntity>(doc->findEntity(rect->id()));
    QVERIFY(reloadedRect != nullptr);
    QCOMPARE(reloadedRect->topLeft(), QPointF(300.0, 100.0));
    QCOMPARE(reloadedRect->width(), 80.0);
    QCOMPARE(reloadedRect->height(), 40.0);

    // Cleanup
    QFile::remove(tempFilePath);
    window.close();
}

#include "FunctionalE2ETest.moc"

QObject* createFunctionalE2ETest() {
    return new FunctionalE2ETest();
}
