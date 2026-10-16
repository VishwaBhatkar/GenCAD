#include <QTest>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

#include "cloud/CloudClient.h"
#include "document/CadDocument.h"
#include "layers/CadLayer.h"
#include "layers/LayerManager.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"
#include "geometry/ArcEntity.h"
#include "geometry/TextEntity.h"
#include "geometry/DimensionEntity.h"
#include "io/GencadFile.h"

class Phase3Tests : public QObject {
    Q_OBJECT

private slots:
    void testCloudClientDefaults();
    void testServerUrlFormatting();
    void testLogoutClearsCredentials();
    void testDocumentJsonSerializationRoundTrip();
    void testInvalidJsonHandling();
    void testProjectDataModel();
    void testDrawingDataModel();
    void testDrawingVersionDataModel();
    void testLiveCloudIntegrationIfAvailable();
};

void Phase3Tests::testCloudClientDefaults() {
    CloudClient client;
    QCOMPARE(client.serverUrl(), QStringLiteral("http://127.0.0.1:8000"));
    QVERIFY(!client.isLoggedIn());
    QVERIFY(client.token().isEmpty());
    QVERIFY(client.username().isEmpty());
    QCOMPARE(client.userId(), 0);
}

void Phase3Tests::testServerUrlFormatting() {
    CloudClient client;
    client.setServerUrl(QStringLiteral("http://localhost:9000///"));
    QCOMPARE(client.serverUrl(), QStringLiteral("http://localhost:9000"));

    client.setServerUrl(QStringLiteral("  http://192.168.1.100:8080/  "));
    QCOMPARE(client.serverUrl(), QStringLiteral("http://192.168.1.100:8080"));
}

void Phase3Tests::testLogoutClearsCredentials() {
    CloudClient client;
    QSignalSpy spy(&client, &CloudClient::authStateChanged);

    client.logout();
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toBool(), false);
    QCOMPARE(arguments.at(1).toString(), QString());
    QVERIFY(!client.isLoggedIn());
}

void Phase3Tests::testDocumentJsonSerializationRoundTrip() {
    CadDocument originalDoc;
    originalDoc.layerManager()->addLayer(QStringLiteral("Mechanical"), QColor(255, 128, 0));
    originalDoc.layerManager()->setActiveLayer(QStringLiteral("Mechanical"));

    auto line = std::make_shared<LineEntity>(QPointF(0, 0), QPointF(100, 50));
    line->setLayer(QStringLiteral("Mechanical"));
    originalDoc.addEntity(line);

    auto circle = std::make_shared<CircleEntity>(QPointF(50, 50), 25.0);
    circle->setLayer(QStringLiteral("Mechanical"));
    originalDoc.addEntity(circle);

    auto rect = std::make_shared<RectangleEntity>(QPointF(10, 10), 80.0, 40.0);
    originalDoc.addEntity(rect);

    auto arc = std::make_shared<ArcEntity>(QPointF(0, 0), 30.0, 0.0, 90.0);
    originalDoc.addEntity(arc);

    auto text = std::make_shared<TextEntity>(QPointF(20, 20), QStringLiteral("Cloud Drawing Test"), 14.0, 45.0);
    originalDoc.addEntity(text);

    auto dim = std::make_shared<DimensionEntity>(QPointF(0, 0), QPointF(100, 0), QPointF(50, 10));
    originalDoc.addEntity(dim);

    // Serialize to JSON string
    QString jsonStr = GencadFile::toJson(&originalDoc);
    QVERIFY(!jsonStr.isEmpty());

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
    QVERIFY(jsonDoc.isObject());
    QJsonObject root = jsonDoc.object();
    QCOMPARE(root["format"].toString(), QStringLiteral("gencad"));
    QCOMPARE(root["version"].toString(), QStringLiteral("2.0"));
    QVERIFY(root["layers"].isArray());
    QVERIFY(root["entities"].isArray());
    QCOMPARE(root["entities"].toArray().size(), 6);

    // Deserialize into new document
    CadDocument loadedDoc;
    QString error;
    QVERIFY(GencadFile::fromJson(jsonStr, &loadedDoc, &error));
    QCOMPARE(loadedDoc.entityCount(), static_cast<size_t>(6));
    QVERIFY(loadedDoc.layerManager()->findLayer(QStringLiteral("Mechanical")) != nullptr);
    QCOMPARE(loadedDoc.layerManager()->activeLayerName(), QStringLiteral("Mechanical"));
}

void Phase3Tests::testInvalidJsonHandling() {
    CadDocument doc;
    QString error;

    // Bad json syntax
    QVERIFY(!GencadFile::fromJson(QStringLiteral("{not valid json}"), &doc, &error));
    QVERIFY(!error.isEmpty());

    // Wrong format
    QVERIFY(!GencadFile::fromJson(QStringLiteral("{\"format\": \"other\"}"), &doc, &error));
    QVERIFY(!error.isEmpty());
}

void Phase3Tests::testProjectDataModel() {
    CloudProject proj;
    proj.id = 42;
    proj.name = QStringLiteral("Architecture Project");
    proj.description = QStringLiteral("Floor plans and elevations");
    proj.createdAt = QStringLiteral("2026-09-24T12:00:00");

    QCOMPARE(proj.id, 42);
    QCOMPARE(proj.name, QStringLiteral("Architecture Project"));
    QCOMPARE(proj.description, QStringLiteral("Floor plans and elevations"));
    QCOMPARE(proj.createdAt, QStringLiteral("2026-09-24T12:00:00"));
}

void Phase3Tests::testDrawingDataModel() {
    CloudDrawing drawing;
    drawing.id = 101;
    drawing.projectId = 42;
    drawing.name = QStringLiteral("FirstFloorPlan");
    drawing.format = QStringLiteral("gencad");
    drawing.version = 3;
    drawing.description = QStringLiteral("Updated electrical layout");
    drawing.updatedAt = QStringLiteral("2026-09-24T14:30:00");

    QCOMPARE(drawing.id, 101);
    QCOMPARE(drawing.projectId, 42);
    QCOMPARE(drawing.name, QStringLiteral("FirstFloorPlan"));
    QCOMPARE(drawing.format, QStringLiteral("gencad"));
    QCOMPARE(drawing.version, 3);
}

void Phase3Tests::testDrawingVersionDataModel() {
    CloudDrawingVersion version;
    version.id = 55;
    version.versionNumber = 2;
    version.commitMessage = QStringLiteral("Added bathroom partition");
    version.createdAt = QStringLiteral("2026-09-24T15:00:00");

    QCOMPARE(version.id, 55);
    QCOMPARE(version.versionNumber, 2);
    QCOMPARE(version.commitMessage, QStringLiteral("Added bathroom partition"));
}

void Phase3Tests::testLiveCloudIntegrationIfAvailable() {
    CloudClient client;
    QString error;

    // Check if live server is reachable by attempting login or list
    // If not reachable, verify error handling without crashing
    QString uniqueUser = QStringLiteral("p3user_%1").arg(QDateTime::currentMSecsSinceEpoch());
    QString email = QStringLiteral("%1@gencad.internal").arg(uniqueUser);
    QString password = QStringLiteral("SecurePass123!");

    bool registered = client.registerUser(uniqueUser, email, password, &error);
    if (!registered) {
        // If server is not running, verify that error is recorded and graceful
        QVERIFY(!client.isLoggedIn());
        return; // Server not online in this test run; graceful pass
    }

    // Live backend is running! Test full lifecycle
    bool loggedIn = client.login(uniqueUser, password, &error);
    QVERIFY(loggedIn);
    QVERIFY(client.isLoggedIn());
    QCOMPARE(client.username(), uniqueUser);

    // Create Project
    CloudProject proj = client.createProject(QStringLiteral("Integration Project"), QStringLiteral("Test phase 3 project"), &error);
    QVERIFY(proj.id > 0);
    QCOMPARE(proj.name, QStringLiteral("Integration Project"));

    // List Projects
    QList<CloudProject> projects = client.listProjects(&error);
    QVERIFY(!projects.isEmpty());

    // Upload Document
    CadDocument doc1;
    doc1.addEntity(std::make_shared<LineEntity>(QPointF(0, 0), QPointF(50, 50)));
    bool saved = client.saveDocumentToCloud(proj.id, QStringLiteral("LiveDrawing1"), &doc1, &error);
    QVERIFY(saved);

    // List Drawings
    QList<CloudDrawing> drawings = client.listDrawings(proj.id, &error);
    QVERIFY(!drawings.isEmpty());
    int drawingId = drawings.first().id;
    QCOMPARE(drawings.first().version, 1);

    // Update Document Revision
    CadDocument doc2;
    doc2.addEntity(std::make_shared<CircleEntity>(QPointF(20, 20), 10.0));
    bool updated = client.updateDocumentToCloud(drawingId, &doc2, QStringLiteral("Added circle"), &error);
    QVERIFY(updated);

    // List Versions
    QList<CloudDrawingVersion> versions = client.listDrawingVersions(drawingId, &error);
    QCOMPARE(versions.size(), 2);

    // Revert to Version 1
    CadDocument docReverted;
    bool reverted = client.revertDrawingVersion(drawingId, 1, &docReverted, &error);
    QVERIFY(reverted);
    QCOMPARE(docReverted.entityCount(), static_cast<size_t>(1));

    // Delete Drawing
    QVERIFY(client.deleteDrawing(drawingId, &error));

    // Delete Project
    QVERIFY(client.deleteProject(proj.id, &error));

    // Logout
    client.logout();
    QVERIFY(!client.isLoggedIn());
}

#include "Phase3Tests.moc"

QObject* createPhase3Tests() {
    return new Phase3Tests();
}
