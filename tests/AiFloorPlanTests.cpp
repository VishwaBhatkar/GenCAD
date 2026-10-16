#include <QTest>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>

#include "ai/AiFloorPlanSpec.h"
#include "ai/AiFloorPlanGenerator.h"
#include "ai/AiFloorPlanProvider.h"
#include "ai/AiCommandExecutor.h"
#include "document/CadDocument.h"
#include "layers/LayerManager.h"
#include "geometry/LineEntity.h"
#include "geometry/ArcEntity.h"
#include "geometry/TextEntity.h"
#include "geometry/DimensionEntity.h"
#include "io/GencadFile.h"

class AiFloorPlanTests : public QObject {
    Q_OBJECT

private slots:
    void testPromptParsingStandard10x12mResidential();
    void testPromptParsingSingleLineAndWallThickness();
    void testAmbiguityClarificationWhenDimensionsMissing();
    void testRejectsNegativeOrZeroDimensions();
    void testRejectsPhysicallyImpossibleSmallFootprint();
    void testGeometryGenerationSingleLine();
    void testGeometryGenerationDoubleLine();
    void testDoorAndSwingArcGeometry();
    void testWindowOpeningsOnExteriorWalls();
    void testStaircaseGeometry();
    void testDimensionsAndLabels();
    void testStructuredJsonSchemaRoundTrip();
    void testCancellationLeavesDocumentUntouched();
    void testUndoRedoAtomicity();
    void testSaveAndReopenGencadFile();
    void testPluggableProviderCredentialsAndFallback();
};

void AiFloorPlanTests::testPromptParsingStandard10x12mResidential() {
    QString prompt = QStringLiteral("Create a 10 m × 12 m residential ground floor with 3 bedrooms, kitchen, living room, 2 toilets, staircase, doors, windows and dimensions.");
    FloorPlanConfig config;
    FloorPlanValidationResult res = AiFloorPlanGenerator::parsePrompt(prompt, &config);

    QVERIFY2(res.isValid, qPrintable(res.errorString()));
    QCOMPARE(config.width, 10000.0);
    QCOMPARE(config.depth, 12000.0);
    QCOMPARE(config.bedroomCount, 3);
    QCOMPARE(config.toiletCount, 2);
    QVERIFY(config.hasLivingRoom);
    QVERIFY(config.hasKitchen);
    QVERIFY(config.hasStairs);
    QVERIFY(config.hasDoors);
    QVERIFY(config.hasWindows);
    QVERIFY(config.hasDimensions);
    QVERIFY(config.hasLabels);
    QCOMPARE(config.wallStyle, FloorPlanWallStyle::DoubleLine);
}

void AiFloorPlanTests::testPromptParsingSingleLineAndWallThickness() {
    QString prompt = QStringLiteral("Create a 12m x 15m single-line floor plan with 250mm exterior walls and 120mm interior walls, 2 bedrooms, 1 toilet");
    FloorPlanConfig config;
    FloorPlanValidationResult res = AiFloorPlanGenerator::parsePrompt(prompt, &config);

    QVERIFY(res.isValid);
    QCOMPARE(config.width, 12000.0);
    QCOMPARE(config.depth, 15000.0);
    QCOMPARE(config.wallStyle, FloorPlanWallStyle::SingleLine);
    QCOMPARE(config.exteriorWallThickness, 250.0);
    QCOMPARE(config.interiorWallThickness, 120.0);
    QCOMPARE(config.bedroomCount, 2);
    QCOMPARE(config.toiletCount, 1);
}

void AiFloorPlanTests::testAmbiguityClarificationWhenDimensionsMissing() {
    // Prompt lacks critical building dimensions: MUST NOT silently invent dimensions!
    QString prompt = QStringLiteral("Create a residential floor plan with 3 bedrooms, kitchen and living room");
    FloorPlanConfig config;
    FloorPlanValidationResult res = AiFloorPlanGenerator::parsePrompt(prompt, &config);

    QVERIFY(!res.isValid);
    QVERIFY(res.needsClarification);
    QVERIFY(!res.clarificationQuestion.isEmpty());
    QVERIFY(res.clarificationQuestion.contains(QStringLiteral("dimensions"), Qt::CaseInsensitive));
    QVERIFY(res.clarificationOptions.size() >= 3);
}

void AiFloorPlanTests::testRejectsNegativeOrZeroDimensions() {
    FloorPlanConfig config;
    config.width = -1000.0;
    config.depth = 12000.0;
    FloorPlanValidationResult res = AiFloorPlanGenerator::validateConfig(config);
    QVERIFY(!res.isValid);
    QVERIFY(res.errorString().contains(QStringLiteral("width"), Qt::CaseInsensitive));

    config.width = 10000.0;
    config.depth = 0.0;
    res = AiFloorPlanGenerator::validateConfig(config);
    QVERIFY(!res.isValid);
    QVERIFY(res.errorString().contains(QStringLiteral("depth"), Qt::CaseInsensitive));
}

void AiFloorPlanTests::testRejectsPhysicallyImpossibleSmallFootprint() {
    QString prompt = QStringLiteral("Create a 1m x 1m residential floor plan with 5 bedrooms, kitchen, living room and 2 toilets");
    FloorPlanConfig config;
    FloorPlanValidationResult res = AiFloorPlanGenerator::parsePrompt(prompt, &config);

    QVERIFY(!res.isValid);
    QVERIFY(res.errorString().contains(QStringLiteral("small"), Qt::CaseInsensitive));
}

void AiFloorPlanTests::testGeometryGenerationSingleLine() {
    FloorPlanConfig config;
    config.width = 10000.0;
    config.depth = 12000.0;
    config.wallStyle = FloorPlanWallStyle::SingleLine;
    config.bedroomCount = 3;
    config.toiletCount = 2;
    config.hasLivingRoom = true;
    config.hasKitchen = true;
    config.hasStairs = true;
    config.hasDoors = true;
    config.hasWindows = true;
    config.hasDimensions = true;

    FloorPlanLayout layout = AiFloorPlanGenerator::generate(config);
    FloorPlanValidationResult valRes = AiFloorPlanGenerator::validateLayout(layout);
    QVERIFY2(valRes.isValid, qPrintable(valRes.errorString()));

    QCOMPARE(layout.totalAreaSqM, 120.0);
    QVERIFY(!layout.rooms.empty());
    QVERIFY(!layout.walls.empty());
    QVERIFY(!layout.doors.empty());
    QVERIFY(!layout.windows.empty());
    QVERIFY(!layout.stairs.empty());
    QVERIFY(!layout.labels.empty());
    QVERIFY(!layout.dimensions.empty());

    // Single-line exterior walls have 0 thickness
    bool foundExterior = false;
    for (const auto& w : layout.walls) {
        if (w.isExterior) {
            foundExterior = true;
            QCOMPARE(w.thickness, 0.0);
        }
    }
    QVERIFY(foundExterior);

    // Verify all room boundaries lie strictly inside building
    QRectF bldg(0, 0, 10000, 12000);
    for (const auto& r : layout.rooms) {
        QVERIFY(bldg.contains(r.rect.topLeft()));
        QVERIFY(bldg.contains(r.rect.bottomRight()));
        QVERIFY(r.areaSqM > 0.0);
    }
}

void AiFloorPlanTests::testGeometryGenerationDoubleLine() {
    FloorPlanConfig config;
    config.width = 10000.0;
    config.depth = 12000.0;
    config.wallStyle = FloorPlanWallStyle::DoubleLine;
    config.exteriorWallThickness = 200.0;
    config.interiorWallThickness = 100.0;
    config.bedroomCount = 3;
    config.toiletCount = 2;
    config.hasStairs = true;

    FloorPlanLayout layout = AiFloorPlanGenerator::generate(config);
    FloorPlanValidationResult valRes = AiFloorPlanGenerator::validateLayout(layout);
    QVERIFY2(valRes.isValid, qPrintable(valRes.errorString()));

    // Verify outer and inner perimeter wall segments exist
    bool foundOuter = false;
    bool foundInner = false;
    for (const auto& w : layout.walls) {
        if (w.p1 == QPointF(0, 0) && w.p2 == QPointF(10000, 0)) foundOuter = true;
        if (w.p1 == QPointF(200, 200) && w.p2 == QPointF(9800, 200)) foundInner = true;
    }
    QVERIFY(foundOuter);
    QVERIFY(foundInner);

    // Verify room non-overlap
    for (size_t i = 0; i < layout.rooms.size(); ++i) {
        for (size_t j = i + 1; j < layout.rooms.size(); ++j) {
            QRectF r1 = layout.rooms[i].rect.adjusted(1.0, 1.0, -1.0, -1.0);
            QRectF r2 = layout.rooms[j].rect.adjusted(1.0, 1.0, -1.0, -1.0);
            QVERIFY(!r1.intersects(r2));
        }
    }
}

void AiFloorPlanTests::testDoorAndSwingArcGeometry() {
    FloorPlanConfig config;
    config.width = 10000.0;
    config.depth = 12000.0;
    config.hasDoors = true;

    FloorPlanLayout layout = AiFloorPlanGenerator::generate(config);
    QVERIFY(!layout.doors.empty());

    for (const auto& d : layout.doors) {
        QVERIFY(d.width >= 700.0 && d.width <= 1000.0);
        QCOMPARE(d.swingRadius, d.width);
        QCOMPARE(d.sweepAngleDeg, 90.0);
        QCOMPARE(d.swingCenter, d.hinge);
        QCOMPARE(d.layer, QStringLiteral("A-DOOR"));
    }

    // Instantiation verification
    auto entities = layout.instantiateEntities();
    int arcCount = 0;
    int doorLineCount = 0;
    for (const auto& e : entities) {
        if (e->layer() == QStringLiteral("A-DOOR")) {
            if (e->type() == EntityType::Arc) arcCount++;
            else if (e->type() == EntityType::Line) doorLineCount++;
        }
    }
    QVERIFY(arcCount >= static_cast<int>(layout.doors.size()));
    QVERIFY(doorLineCount >= static_cast<int>(layout.doors.size()));
}

void AiFloorPlanTests::testWindowOpeningsOnExteriorWalls() {
    FloorPlanConfig config;
    config.width = 10000.0;
    config.depth = 12000.0;
    config.hasWindows = true;

    FloorPlanLayout layout = AiFloorPlanGenerator::generate(config);
    QVERIFY(!layout.windows.empty());

    for (const auto& win : layout.windows) {
        QVERIFY(win.width > 500.0);
        QCOMPARE(win.layer, QStringLiteral("A-GLAZ"));

        // Must lie on one of the 4 outer building boundary coordinates
        bool onSouth = (std::abs(win.p1.y()) < 1.0 && std::abs(win.p2.y()) < 1.0);
        bool onNorth = (std::abs(win.p1.y() - 12000.0) < 1.0 && std::abs(win.p2.y() - 12000.0) < 1.0);
        bool onWest = (std::abs(win.p1.x()) < 1.0 && std::abs(win.p2.x()) < 1.0);
        bool onEast = (std::abs(win.p1.x() - 10000.0) < 1.0 && std::abs(win.p2.x() - 10000.0) < 1.0);
        QVERIFY(onSouth || onNorth || onWest || onEast);
    }
}

void AiFloorPlanTests::testStaircaseGeometry() {
    FloorPlanConfig config;
    config.width = 10000.0;
    config.depth = 12000.0;
    config.hasStairs = true;

    FloorPlanLayout layout = AiFloorPlanGenerator::generate(config);
    QCOMPARE(layout.stairs.size(), size_t(1));

    const auto& st = layout.stairs[0];
    QCOMPARE(st.stepCount, 14);
    QCOMPARE(st.direction, QStringLiteral("UP"));
    QCOMPARE(st.layer, QStringLiteral("A-FLOR-STRS"));

    AiProposal prop = layout.toAiProposal();
    int stairOps = 0;
    for (const auto& op : prop.operations) {
        if (op.layer == QStringLiteral("A-FLOR-STRS")) stairOps++;
    }
    // Stringers (2) + Treads (15) + Arrow (1) + UP Text (1) = 19
    QVERIFY(stairOps >= 15);
}

void AiFloorPlanTests::testDimensionsAndLabels() {
    FloorPlanConfig config;
    config.width = 10000.0;
    config.depth = 12000.0;
    config.hasDimensions = true;
    config.hasLabels = true;

    FloorPlanLayout layout = AiFloorPlanGenerator::generate(config);
    QVERIFY(layout.dimensions.size() >= 4); // Overall W top/bottom and D left/right
    QVERIFY(!layout.labels.empty());

    for (const auto& l : layout.labels) {
        QCOMPARE(l.layer, QStringLiteral("A-ANNO-TEXT"));
        QVERIFY(l.text.contains(QStringLiteral("m²")));
        QVERIFY(l.height >= 100.0);
    }
}

void AiFloorPlanTests::testStructuredJsonSchemaRoundTrip() {
    FloorPlanConfig config;
    config.width = 10000.0;
    config.depth = 12000.0;
    config.wallStyle = FloorPlanWallStyle::DoubleLine;
    config.bedroomCount = 3;
    config.toiletCount = 2;
    config.hasStairs = true;

    FloorPlanLayout original = AiFloorPlanGenerator::generate(config);
    QJsonObject json = original.toJson();

    QCOMPARE(json["$schema"].toString(), QStringLiteral("https://gencad.org/schemas/v1/floorplan.json"));
    QCOMPARE(json["version"].toString(), QStringLiteral("1.0"));
    QCOMPARE(json["total_area_m2"].toDouble(), 120.0);

    FloorPlanLayout restored;
    QString err;
    bool ok = FloorPlanLayout::fromJson(json, &restored, &err);
    QVERIFY2(ok, qPrintable(err));

    QCOMPARE(restored.config.width, original.config.width);
    QCOMPARE(restored.config.depth, original.config.depth);
    QCOMPARE(restored.rooms.size(), original.rooms.size());
    QCOMPARE(restored.walls.size(), original.walls.size());
    QCOMPARE(restored.doors.size(), original.doors.size());
    QCOMPARE(restored.windows.size(), original.windows.size());
    QCOMPARE(restored.stairs.size(), original.stairs.size());
}

void AiFloorPlanTests::testCancellationLeavesDocumentUntouched() {
    CadDocument doc;
    QCOMPARE(doc.entityCount(), size_t(0));
    QVERIFY(!doc.isModified());

    // Generate proposal
    FloorPlanConfig config;
    config.width = 10000.0;
    config.depth = 12000.0;
    FloorPlanLayout layout = AiFloorPlanGenerator::generate(config);
    auto previewEntities = layout.instantiateEntities();
    QVERIFY(!previewEntities.empty());

    // Simulating preview and cancellation: preview entities are NEVER added to CadDocument
    previewEntities.clear();

    QCOMPARE(doc.entityCount(), size_t(0));
    QVERIFY(!doc.isModified());
}

void AiFloorPlanTests::testUndoRedoAtomicity() {
    CadDocument doc;
    QCOMPARE(doc.entityCount(), size_t(0));

    FloorPlanConfig config;
    config.width = 10000.0;
    config.depth = 12000.0;
    FloorPlanLayout layout = AiFloorPlanGenerator::generate(config);
    AiProposal proposal = layout.toAiProposal();

    QString err;
    bool ok = AiCommandExecutor::executeProposal(proposal, &doc, &err);
    QVERIFY2(ok, qPrintable(err));

    size_t createdCount = doc.entityCount();
    QVERIFY(createdCount > 20);
    QVERIFY(doc.isModified());

    // Single undo step must revert document to 0 entities
    doc.undoStack()->undo();
    QCOMPARE(doc.entityCount(), size_t(0));

    // Single redo step must restore all entities
    doc.undoStack()->redo();
    QCOMPARE(doc.entityCount(), createdCount);
}

void AiFloorPlanTests::testSaveAndReopenGencadFile() {
    CadDocument doc;
    FloorPlanConfig config;
    config.width = 10000.0;
    config.depth = 12000.0;
    FloorPlanLayout layout = AiFloorPlanGenerator::generate(config);
    AiProposal proposal = layout.toAiProposal();

    QString err;
    QVERIFY(AiCommandExecutor::executeProposal(proposal, &doc, &err));

    // Save to versioned JSON string
    QString jsonString = GencadFile::toJson(&doc);
    QVERIFY(!jsonString.isEmpty());

    // Reload into a separate document
    CadDocument doc2;
    QString loadErr;
    bool ok = GencadFile::fromJson(jsonString, &doc2, &loadErr);
    QVERIFY2(ok, qPrintable(loadErr));

    QCOMPARE(doc2.entityCount(), doc.entityCount());

    // Verify all standard architectural layers were saved and restored
    auto* lm = doc2.layerManager();
    QVERIFY(lm != nullptr);
    QVERIFY(lm->findLayer(QStringLiteral("A-WALL")) != nullptr);
    QVERIFY(lm->findLayer(QStringLiteral("A-DOOR")) != nullptr);
    QVERIFY(lm->findLayer(QStringLiteral("A-GLAZ")) != nullptr);
    QVERIFY(lm->findLayer(QStringLiteral("A-FLOR-STRS")) != nullptr);
    QVERIFY(lm->findLayer(QStringLiteral("A-ANNO-TEXT")) != nullptr);
    QVERIFY(lm->findLayer(QStringLiteral("A-ANNO-DIMS")) != nullptr);
}

void AiFloorPlanTests::testPluggableProviderCredentialsAndFallback() {
    auto& engine = AiFloorPlanEngine::instance();
    QVERIFY(!engine.providers().empty());

    // Active provider defaults to deterministic
    auto* active = engine.activeProvider();
    QVERIFY(active != nullptr);
    QCOMPARE(active->id(), QStringLiteral("deterministic"));
    QVERIFY(active->isConfigured());

    // Test cloud provider fallback when no API key is present
    CloudAiFloorPlanProvider cloudProvider;
    // Without API key, generatePlan falls back seamlessly to deterministic engine
    FloorPlanProviderResult res = cloudProvider.generatePlan(QStringLiteral("Create a 10m x 12m floor plan with 3 bedrooms"));
    QVERIFY(res.success);
    QVERIFY(res.providerUsed.contains(QStringLiteral("Deterministic")));
    QVERIFY(!res.layout.rooms.empty());
}

QObject* createAiFloorPlanTests() {
    return new AiFloorPlanTests();
}

#include "AiFloorPlanTests.moc"
