#include <QTest>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>

#include "ai/AiDsl.h"
#include "ai/AiValidator.h"
#include "ai/AiCommandExecutor.h"
#include "ai/AiGeometryAnalyzer.h"
#include "ai/AiDimensionGenerator.h"
#include "ai/AiRagEngine.h"
#include "ai/AiLocalEngine.h"
#include "document/CadDocument.h"
#include "layers/LayerManager.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"
#include "geometry/DimensionEntity.h"

class Phase4Tests : public QObject {
    Q_OBJECT

private slots:
    void testDslSerializationRoundTrip();
    void testValidatorValidOperations();
    void testValidatorRejectsNegativeRadius();
    void testValidatorRejectsZeroDimensions();
    void testValidatorRejectsDegenerateLine();
    void testValidatorRejectsNonExistentTargetId();
    void testValidatorRejectsNanCoordinates();
    void testAiCommandExecution();
    void testAiMacroCommandUndoRedoAtomicity();
    void testAiGeometryAnalyzerDuplicates();
    void testAiGeometryAnalyzerOverlaps();
    void testAiCleanupProposalExecution();
    void testAiDimensionGenerator();
    void testAiLocalEngineRoomGeneration();
    void testAiLocalEngineDoorAndStairs();
    void testAiLocalEngineBoltCircle();
    void testAiRagEngineSearchAndAnswer();
    void testHumanApprovalStagingWorkflow();
};

void Phase4Tests::testDslSerializationRoundTrip() {
    AiProposal original;
    original.description = QStringLiteral("Test proposal");
    original.prompt = QStringLiteral("Generate test geometry");

    // Operation 1: Line
    AiOperation opLine;
    opLine.type = AiOperationType::CreateLine;
    opLine.p1 = QPointF(10, 20);
    opLine.p2 = QPointF(100, 200);
    opLine.layer = QStringLiteral("Mechanical");
    original.operations.push_back(opLine);

    // Operation 2: Circle
    AiOperation opCircle;
    opCircle.type = AiOperationType::CreateCircle;
    opCircle.p1 = QPointF(50, 50);
    opCircle.radius = 25.0;
    original.operations.push_back(opCircle);

    // Operation 3: Rectangle
    AiOperation opRect;
    opRect.type = AiOperationType::CreateRectangle;
    opRect.p1 = QPointF(0, 0);
    opRect.width = 500.0;
    opRect.height = 300.0;
    original.operations.push_back(opRect);

    // Operation 4: Arc
    AiOperation opArc;
    opArc.type = AiOperationType::CreateArc;
    opArc.p1 = QPointF(100, 100);
    opArc.radius = 40.0;
    opArc.startAngle = 0.0;
    opArc.sweepAngle = 90.0;
    original.operations.push_back(opArc);

    // Operation 5: Dimension
    AiOperation opDim;
    opDim.type = AiOperationType::CreateDimension;
    opDim.p1 = QPointF(0, 0);
    opDim.p2 = QPointF(500, 0);
    opDim.p3 = QPointF(250, -25);
    original.operations.push_back(opDim);

    QJsonObject json = original.toJson();
    QCOMPARE(json["version"].toString(), QStringLiteral("1.0"));
    QCOMPARE(json["description"].toString(), QStringLiteral("Test proposal"));
    QCOMPARE(json["operations"].toArray().size(), 5);

    AiProposal parsed;
    QString err;
    bool ok = AiProposal::fromJson(json, &parsed, &err);
    QVERIFY2(ok, qPrintable(err));
    QCOMPARE(parsed.operations.size(), original.operations.size());
    QCOMPARE(parsed.createdCount(), 5);
    QCOMPARE(parsed.dimensionCount(), 1);
    QCOMPARE(parsed.operations[0].type, AiOperationType::CreateLine);
    QCOMPARE(parsed.operations[1].type, AiOperationType::CreateCircle);
    QCOMPARE(parsed.operations[2].width, 500.0);
}

void Phase4Tests::testValidatorValidOperations() {
    CadDocument doc;
    AiProposal prop;
    prop.description = QStringLiteral("Valid Proposal");

    AiOperation op;
    op.type = AiOperationType::CreateLine;
    op.p1 = QPointF(0, 0);
    op.p2 = QPointF(100, 100);
    prop.operations.push_back(op);

    AiValidationResult res = AiValidator::validateProposal(prop, &doc);
    QVERIFY(res.isValid);
    QVERIFY(res.errors.isEmpty());
}

void Phase4Tests::testValidatorRejectsNegativeRadius() {
    CadDocument doc;
    AiProposal prop;
    AiOperation op;
    op.type = AiOperationType::CreateCircle;
    op.p1 = QPointF(0, 0);
    op.radius = -10.0; // Invalid negative radius
    prop.operations.push_back(op);

    AiValidationResult res = AiValidator::validateProposal(prop, &doc);
    QVERIFY(!res.isValid);
    QVERIFY(res.errorString().contains(QStringLiteral("radius")));
}

void Phase4Tests::testValidatorRejectsZeroDimensions() {
    CadDocument doc;
    AiProposal prop;
    AiOperation op;
    op.type = AiOperationType::CreateRectangle;
    op.p1 = QPointF(0, 0);
    op.width = 0.0; // Invalid 0 width
    op.height = 100.0;
    prop.operations.push_back(op);

    AiValidationResult res = AiValidator::validateProposal(prop, &doc);
    QVERIFY(!res.isValid);
    QVERIFY(res.errorString().contains(QStringLiteral("width")));
}

void Phase4Tests::testValidatorRejectsDegenerateLine() {
    CadDocument doc;
    AiProposal prop;
    AiOperation op;
    op.type = AiOperationType::CreateLine;
    op.p1 = QPointF(50, 50);
    op.p2 = QPointF(50, 50); // Start == End
    prop.operations.push_back(op);

    AiValidationResult res = AiValidator::validateProposal(prop, &doc);
    QVERIFY(!res.isValid);
    QVERIFY(res.errorString().contains(QStringLiteral("identical")));
}

void Phase4Tests::testValidatorRejectsNonExistentTargetId() {
    CadDocument doc;
    AiProposal prop;
    AiOperation op;
    op.type = AiOperationType::DeleteEntity;
    op.targetId = QUuid::createUuid(); // Random UUID not in document
    prop.operations.push_back(op);

    AiValidationResult res = AiValidator::validateProposal(prop, &doc);
    QVERIFY(!res.isValid);
    QVERIFY(res.errorString().contains(QStringLiteral("does not exist")));
}

void Phase4Tests::testValidatorRejectsNanCoordinates() {
    CadDocument doc;
    AiProposal prop;
    AiOperation op;
    op.type = AiOperationType::CreateLine;
    op.p1 = QPointF(std::numeric_limits<double>::quiet_NaN(), 0.0);
    op.p2 = QPointF(100.0, 100.0);
    prop.operations.push_back(op);

    AiValidationResult res = AiValidator::validateProposal(prop, &doc);
    QVERIFY(!res.isValid);
    QVERIFY(res.errorString().contains(QStringLiteral("finite")));
}

void Phase4Tests::testAiCommandExecution() {
    CadDocument doc;
    QCOMPARE(doc.entityCount(), 0ULL);

    AiProposal prop;
    prop.description = QStringLiteral("Create rectangle 500x300");

    AiOperation op;
    op.type = AiOperationType::CreateRectangle;
    op.p1 = QPointF(0, 0);
    op.width = 500.0;
    op.height = 300.0;
    op.layer = QStringLiteral("Geometry");
    prop.operations.push_back(op);

    QString err;
    bool ok = AiCommandExecutor::executeProposal(prop, &doc, &err);
    QVERIFY2(ok, qPrintable(err));
    QCOMPARE(doc.entityCount(), 1ULL);
    QVERIFY(doc.layerManager()->findLayer(QStringLiteral("Geometry")) != nullptr);

    auto ent = doc.entities().front();
    QCOMPARE(ent->type(), EntityType::Rectangle);
    QCOMPARE(ent->layer(), QStringLiteral("Geometry"));
}

void Phase4Tests::testAiMacroCommandUndoRedoAtomicity() {
    CadDocument doc;
    QCOMPARE(doc.entityCount(), 0ULL);

    AiProposal prop;
    prop.description = QStringLiteral("Atomic multi-entity creation");

    for (int i = 0; i < 4; ++i) {
        AiOperation op;
        op.type = AiOperationType::CreateLine;
        op.p1 = QPointF(i * 10, 0);
        op.p2 = QPointF(i * 10, 100);
        prop.operations.push_back(op);
    }

    QString err;
    bool ok = AiCommandExecutor::executeProposal(prop, &doc, &err);
    QVERIFY(ok);
    QCOMPARE(doc.entityCount(), 4ULL);

    // Single undo must revert ALL 4 entities atomically
    doc.undoStack()->undo();
    QCOMPARE(doc.entityCount(), 0ULL);

    // Single redo must restore ALL 4 entities atomically
    doc.undoStack()->redo();
    QCOMPARE(doc.entityCount(), 4ULL);
}

void Phase4Tests::testAiGeometryAnalyzerDuplicates() {
    CadDocument doc;

    // Line 1 and duplicate Line 2
    auto l1 = std::make_shared<LineEntity>(QPointF(0, 0), QPointF(100, 0));
    auto l2 = std::make_shared<LineEntity>(QPointF(0, 0), QPointF(100, 0)); // duplicate
    doc.addEntity(l1);
    doc.addEntity(l2);

    // Circle 1 and duplicate Circle 2
    auto c1 = std::make_shared<CircleEntity>(QPointF(50, 50), 20.0);
    auto c2 = std::make_shared<CircleEntity>(QPointF(50, 50), 20.0); // duplicate
    doc.addEntity(c1);
    doc.addEntity(c2);

    AiAnalysisReport report = AiGeometryAnalyzer::analyze(&doc);
    QCOMPARE(report.totalEntities, 4);
    QCOMPARE(static_cast<int>(report.duplicates.size()), 2);
    QVERIFY(report.hasIssues());
}

void Phase4Tests::testAiGeometryAnalyzerOverlaps() {
    CadDocument doc;

    // Two colinear overlapping lines
    auto l1 = std::make_shared<LineEntity>(QPointF(0, 0), QPointF(100, 0));
    auto l2 = std::make_shared<LineEntity>(QPointF(50, 0), QPointF(150, 0));
    doc.addEntity(l1);
    doc.addEntity(l2);

    AiAnalysisReport report = AiGeometryAnalyzer::analyze(&doc);
    QCOMPARE(static_cast<int>(report.overlaps.size()), 1);
}

void Phase4Tests::testAiCleanupProposalExecution() {
    CadDocument doc;

    auto l1 = std::make_shared<LineEntity>(QPointF(0, 0), QPointF(100, 0));
    auto l2 = std::make_shared<LineEntity>(QPointF(100, 0), QPointF(0, 0)); // reverse duplicate
    doc.addEntity(l1);
    doc.addEntity(l2);
    QCOMPARE(doc.entityCount(), 2ULL);

    AiProposal cleanupProp = AiGeometryAnalyzer::generateCleanupProposal(&doc);
    QCOMPARE(cleanupProp.deletedCount(), 1);
    QCOMPARE(cleanupProp.operations[0].targetId, l2->id());

    QString err;
    bool ok = AiCommandExecutor::executeProposal(cleanupProp, &doc, &err);
    QVERIFY(ok);
    QCOMPARE(doc.entityCount(), 1ULL);
    QCOMPARE(doc.entities()[0]->id(), l1->id());

    // Undo should restore duplicate
    doc.undoStack()->undo();
    QCOMPARE(doc.entityCount(), 2ULL);
}

void Phase4Tests::testAiDimensionGenerator() {
    CadDocument doc;
    auto rect = std::make_shared<RectangleEntity>(QPointF(0, 0), 200.0, 100.0);
    doc.addEntity(rect);

    AiProposal dimProp = AiDimensionGenerator::generateDimensions(&doc);
    QVERIFY(dimProp.dimensionCount() >= 2);

    QString err;
    bool ok = AiCommandExecutor::executeProposal(dimProp, &doc, &err);
    QVERIFY(ok);
    QVERIFY(doc.entityCount() >= 3ULL);
}

void Phase4Tests::testAiLocalEngineRoomGeneration() {
    AiLocalEngine engine;
    CadDocument doc;

    AiProposal prop = engine.generateFromPrompt(QStringLiteral("Create a 5m x 4m room with 200mm walls"), &doc);
    QVERIFY(!prop.operations.empty());
    QVERIFY(prop.createdCount() >= 4);

    AiValidationResult valRes = AiValidator::validateProposal(prop, &doc);
    QVERIFY(valRes.isValid);

    QString err;
    bool ok = AiCommandExecutor::executeProposal(prop, &doc, &err);
    QVERIFY(ok);

    QRectF bbox = doc.boundingBox();
    // Bounding box includes 5000x4000 walls plus 300mm offset annotation dimensions
    QVERIFY(bbox.width() >= 4900.0 && bbox.width() <= 5500.0);
    QVERIFY(bbox.height() >= 3900.0 && bbox.height() <= 4500.0);

    // Verify outer wall rectangle is exactly 5000 x 4000
    bool foundOuterWall = false;
    for (const auto& ent : doc.entities()) {
        if (ent->type() == EntityType::Rectangle) {
            auto rect = std::dynamic_pointer_cast<RectangleEntity>(ent);
            if (rect && std::abs(rect->width() - 5000.0) < 1.0 && std::abs(rect->height() - 4000.0) < 1.0) {
                foundOuterWall = true;
                break;
            }
        }
    }
    QVERIFY(foundOuterWall);
}

void Phase4Tests::testAiLocalEngineDoorAndStairs() {
    AiLocalEngine engine;
    CadDocument doc;

    AiProposal doorProp = engine.generateFromPrompt(QStringLiteral("Add a 900mm door"), &doc);
    QVERIFY(doorProp.createdCount() >= 2);
    AiValidationResult valDoor = AiValidator::validateProposal(doorProp, &doc);
    QVERIFY(valDoor.isValid);

    AiProposal stairProp = engine.generateFromPrompt(QStringLiteral("Create a staircase with 12 steps"), &doc);
    QVERIFY(stairProp.createdCount() >= 14);
    AiValidationResult valStair = AiValidator::validateProposal(stairProp, &doc);
    QVERIFY(valStair.isValid);
}

void Phase4Tests::testAiLocalEngineBoltCircle() {
    AiLocalEngine engine;
    CadDocument doc;

    AiProposal boltProp = engine.generateFromPrompt(QStringLiteral("Bolt circle with 6 holes radius 100"), &doc);
    QVERIFY(boltProp.createdCount() >= 8); // Pitch circle + bore + 6 holes
    AiValidationResult valBolt = AiValidator::validateProposal(boltProp, &doc);
    QVERIFY(valBolt.isValid);
}

void Phase4Tests::testAiRagEngineSearchAndAnswer() {
    AiRagEngine rag;
    auto results = rag.query(QStringLiteral("How do I use Trim tool?"));
    QVERIFY(!results.empty());
    QVERIFY(results[0].title.contains(QStringLiteral("Modify")) || results[0].category == QStringLiteral("Tools"));

    auto stdResults = rag.query(QStringLiteral("ISO 128 technical drawing line weights"));
    QVERIFY(!stdResults.empty());
    QVERIFY(stdResults[0].title.contains(QStringLiteral("ISO 128")));

    QString answer = rag.answerQuestion(QStringLiteral("What is AIA layer standard?"));
    QVERIFY(answer.contains(QStringLiteral("A-WALL")));
}

void Phase4Tests::testHumanApprovalStagingWorkflow() {
    AiLocalEngine engine;
    CadDocument doc;

    // 1. User prompts AI
    QString prompt = QStringLiteral("Create a rectangle 500 mm x 300 mm");
    AiProposal proposal = engine.generateFromPrompt(prompt, &doc);

    // 2. Proposal is staged, not yet applied
    QCOMPARE(doc.entityCount(), 0ULL);
    QVERIFY(!proposal.summaryText().isEmpty());
    QVERIFY(proposal.summaryText().contains(QStringLiteral("Rectangle")));

    // 3. User reviews and approves (Apply)
    QString err;
    bool ok = AiCommandExecutor::executeProposal(proposal, &doc, &err);
    QVERIFY(ok);
    QCOMPARE(doc.entityCount(), 1ULL);

    // 4. User can cleanly undo the entire proposal
    doc.undoStack()->undo();
    QCOMPARE(doc.entityCount(), 0ULL);
}

QObject* createPhase4Tests() {
    return new Phase4Tests();
}

#include "Phase4Tests.moc"
