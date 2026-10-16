#include <QTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QImage>
#include <QFileInfo>
#include <QFile>

#include "document/CadDocument.h"
#include "document/UnitSystem.h"
#include "document/PageSettings.h"
#include "layers/LayerManager.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"
#include "geometry/ArcEntity.h"
#include "geometry/PolylineEntity.h"
#include "geometry/PolygonEntity.h"
#include "geometry/EllipseEntity.h"
#include "geometry/TextEntity.h"
#include "geometry/DimensionEntity.h"
#include "io/GencadFile.h"
#include "io/DxfFile.h"
#include "io/DrawingExporter.h"

class FileIoTests : public QObject {
    Q_OBJECT

private slots:
    void testSaveAndLoad();
    void testEmptyDocumentSaveLoad();
    void testInvalidFileHandling();
    void testRejectDxfDwgInGencadFile();
    void testDxfExportAndImportAllEntities();
    void testDxfEmptyDrawingError();
    void testPdfExport();
    void testPngExport();
    void testDrawingExporterFormatDetection();
    void testExportDoesNotModifyDocumentState();
};

void FileIoTests::testSaveAndLoad() {
    CadDocument originalDoc;
    auto line = std::make_shared<LineEntity>(QPointF(10.5, 20.5), QPointF(100.25, 200.75));
    auto circle = std::make_shared<CircleEntity>(QPointF(50.0, 50.0), 30.0);
    auto rect = std::make_shared<RectangleEntity>(QPointF(10.0, 10.0), 40.0, 60.0);

    QUuid lineId = line->id();
    QUuid circleId = circle->id();
    QUuid rectId = rect->id();

    originalDoc.addEntity(line);
    originalDoc.addEntity(circle);
    originalDoc.addEntity(rect);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString tempPath = tempFile.fileName();
    tempFile.close();

    // Save
    QString error;
    QVERIFY(GencadFile::save(tempPath, &originalDoc, &error));
    QVERIFY(error.isEmpty());

    // Load into new document
    CadDocument loadedDoc;
    QVERIFY(GencadFile::load(tempPath, &loadedDoc, &error));
    QVERIFY(error.isEmpty());

    QCOMPARE(loadedDoc.entityCount(), 3ULL);

    auto loadedLine = std::dynamic_pointer_cast<LineEntity>(loadedDoc.findEntity(lineId));
    QVERIFY(loadedLine != nullptr);
    QCOMPARE(loadedLine->p1(), QPointF(10.5, 20.5));
    QCOMPARE(loadedLine->p2(), QPointF(100.25, 200.75));

    auto loadedCircle = std::dynamic_pointer_cast<CircleEntity>(loadedDoc.findEntity(circleId));
    QVERIFY(loadedCircle != nullptr);
    QCOMPARE(loadedCircle->center(), QPointF(50.0, 50.0));
    QCOMPARE(loadedCircle->radius(), 30.0);

    auto loadedRect = std::dynamic_pointer_cast<RectangleEntity>(loadedDoc.findEntity(rectId));
    QVERIFY(loadedRect != nullptr);
    QCOMPARE(loadedRect->topLeft(), QPointF(10.0, 10.0));
    QCOMPARE(loadedRect->width(), 40.0);
    QCOMPARE(loadedRect->height(), 60.0);
}

void FileIoTests::testEmptyDocumentSaveLoad() {
    CadDocument emptyDoc;
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString tempPath = tempFile.fileName();
    tempFile.close();

    QString error;
    QVERIFY(GencadFile::save(tempPath, &emptyDoc, &error));

    CadDocument loadedDoc;
    QVERIFY(GencadFile::load(tempPath, &loadedDoc, &error));
    QCOMPARE(loadedDoc.entityCount(), 0ULL);
}

void FileIoTests::testInvalidFileHandling() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    tempFile.write("This is not valid JSON content");
    QString tempPath = tempFile.fileName();
    tempFile.close();

    CadDocument doc;
    QString error;
    QVERIFY(!GencadFile::load(tempPath, &doc, &error));
    QVERIFY(!error.isEmpty());
}

void FileIoTests::testRejectDxfDwgInGencadFile() {
    CadDocument doc;
    doc.addEntity(std::make_shared<LineEntity>(QPointF(0, 0), QPointF(10, 10)));

    QString error;
    // Attempting to save JSON directly with .dxf extension must be rejected
    QVERIFY(!GencadFile::save(QStringLiteral("test.dxf"), &doc, &error));
    QVERIFY(error.contains(QStringLiteral(".dxf"), Qt::CaseInsensitive));

    // Attempting to save JSON directly with .dwg extension must be rejected
    QVERIFY(!GencadFile::save(QStringLiteral("test.dwg"), &doc, &error));
    QVERIFY(error.contains(QStringLiteral(".dwg"), Qt::CaseInsensitive));

    // Attempting to load .dxf or .dwg through GencadFile must be rejected
    QVERIFY(!GencadFile::load(QStringLiteral("test.dxf"), &doc, &error));
    QVERIFY(!GencadFile::load(QStringLiteral("test.dwg"), &doc, &error));
}

void FileIoTests::testDxfExportAndImportAllEntities() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString dxfPath = tempDir.filePath(QStringLiteral("all_entities.dxf"));

    CadDocument doc;
    doc.unitSettings().setType(UnitType::Millimeter);
    doc.layerManager()->addLayer(QStringLiteral("Mechanical"), QColor(255, 0, 0));
    doc.layerManager()->addLayer(QStringLiteral("Annotation"), QColor(0, 255, 0));

    // 1. Line
    auto line = std::make_shared<LineEntity>(QPointF(0, 0), QPointF(100, 100));
    line->setLayer(QStringLiteral("Mechanical"));
    doc.addEntity(line);

    // 2. Circle
    auto circle = std::make_shared<CircleEntity>(QPointF(50, 50), 25);
    circle->setLayer(QStringLiteral("Mechanical"));
    doc.addEntity(circle);

    // 3. Rectangle
    auto rect = std::make_shared<RectangleEntity>(QPointF(10, 10), 80, 40);
    rect->setLayer(QStringLiteral("Mechanical"));
    doc.addEntity(rect);

    // 4. Arc
    auto arc = std::make_shared<ArcEntity>(QPointF(60, 60), 30, 45, 90);
    arc->setLayer(QStringLiteral("Mechanical"));
    doc.addEntity(arc);

    // 5. Polyline
    std::vector<QPointF> polyPts = { QPointF(0, 0), QPointF(50, 20), QPointF(80, 60) };
    auto poly = std::make_shared<PolylineEntity>(polyPts, false);
    poly->setLayer(QStringLiteral("Mechanical"));
    doc.addEntity(poly);

    // 6. Polygon
    auto polygon = std::make_shared<PolygonEntity>(QPointF(120, 120), 6, 30.0);
    polygon->setLayer(QStringLiteral("Mechanical"));
    doc.addEntity(polygon);

    // 7. Ellipse
    auto ellipse = std::make_shared<EllipseEntity>(QPointF(150, 150), 40.0, 20.0, 30.0);
    ellipse->setLayer(QStringLiteral("Mechanical"));
    doc.addEntity(ellipse);

    // 8. Text
    auto text = std::make_shared<TextEntity>(QPointF(20, 200), QStringLiteral("GenCAD DXF"), 12.0);
    text->setLayer(QStringLiteral("Annotation"));
    doc.addEntity(text);

    // 9. Dimension
    auto dim = std::make_shared<DimensionEntity>(QPointF(0, 0), QPointF(100, 0), QPointF(50, 20));
    dim->setLayer(QStringLiteral("Annotation"));
    doc.addEntity(dim);

    QCOMPARE(doc.entityCount(), 9ULL);

    // Save DXF
    QString error;
    QStringList warnings;
    QVERIFY2(DxfFile::save(dxfPath, &doc, &error, &warnings), qPrintable(error));

    // Verify file content is valid ASCII DXF
    QFile f(dxfPath);
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = QString::fromUtf8(f.readAll());
    f.close();
    QVERIFY(content.contains(QStringLiteral("SECTION")));
    QVERIFY(content.contains(QStringLiteral("HEADER")));
    QVERIFY(content.contains(QStringLiteral("ENTITIES")));
    QVERIFY(content.contains(QStringLiteral("EOF")));
    QVERIFY(!content.trimmed().startsWith(QStringLiteral("{"))); // Must NOT be JSON!

    // Import DXF
    CadDocument importedDoc;
    QVERIFY2(DxfFile::load(dxfPath, &importedDoc, &error, &warnings), qPrintable(error));

    // Verify entity count
    QCOMPARE(importedDoc.entityCount(), 9ULL);

    // Verify layers were restored
    QVERIFY(importedDoc.layerManager()->findLayer(QStringLiteral("Mechanical")) != nullptr);
    QVERIFY(importedDoc.layerManager()->findLayer(QStringLiteral("Annotation")) != nullptr);

    // Verify document filePath is empty (prevents accidental JSON save into DXF)
    QVERIFY(importedDoc.filePath().isEmpty());
}

void FileIoTests::testDxfEmptyDrawingError() {
    CadDocument emptyDoc;
    QString error;
    QVERIFY(!DxfFile::save(QStringLiteral("empty.dxf"), &emptyDoc, &error));
    QVERIFY(error.contains(QStringLiteral("empty"), Qt::CaseInsensitive));
}

void FileIoTests::testPdfExport() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString pdfPath = tempDir.filePath(QStringLiteral("drawing.pdf"));

    CadDocument doc;
    doc.pageSettings().setPageSize(PageSize::A4);
    doc.pageSettings().setOrientation(PageOrientation::Landscape);
    doc.addEntity(std::make_shared<LineEntity>(QPointF(10, 10), QPointF(200, 150)));
    doc.addEntity(std::make_shared<CircleEntity>(QPointF(100, 80), 40.0));
    doc.addEntity(std::make_shared<RectangleEntity>(QPointF(20, 20), 160.0, 100.0));
    doc.addEntity(std::make_shared<TextEntity>(QPointF(30, 170), QStringLiteral("PDF Test"), 14.0));

    QString error;
    QStringList warnings;
    QVERIFY2(DrawingExporter::exportPdf(pdfPath, &doc, ExportOptions(), &error, &warnings), qPrintable(error));

    QFileInfo fi(pdfPath);
    QVERIFY(fi.exists());
    QVERIFY(fi.size() > 500); // Valid PDF header and streams

    // Check PDF magic bytes
    QFile f(pdfPath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QByteArray header = f.read(5);
    f.close();
    QCOMPARE(header, QByteArray("%PDF-"));
}

void FileIoTests::testPngExport() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString pngPath = tempDir.filePath(QStringLiteral("drawing.png"));

    CadDocument doc;
    doc.addEntity(std::make_shared<LineEntity>(QPointF(0, 0), QPointF(100, 50)));
    doc.addEntity(std::make_shared<CircleEntity>(QPointF(50, 25), 20.0));

    QString error;
    QStringList warnings;
    QVERIFY2(DrawingExporter::exportPng(pngPath, &doc, ExportOptions(), &error, &warnings), qPrintable(error));

    QFileInfo fi(pngPath);
    QVERIFY(fi.exists());
    QVERIFY(fi.size() > 100);

    QImage img(pngPath);
    QVERIFY(!img.isNull());
    QVERIFY(img.width() > 0);
    QVERIFY(img.height() > 0);
}

void FileIoTests::testDrawingExporterFormatDetection() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    CadDocument doc;
    doc.addEntity(std::make_shared<LineEntity>(QPointF(0, 0), QPointF(50, 50)));

    QString error;
    // DXF export
    QString dxfPath = tempDir.filePath(QStringLiteral("test.dxf"));
    QVERIFY(DrawingExporter::exportDrawing(dxfPath, &doc, ExportOptions(), &error));

    // PDF export
    QString pdfPath = tempDir.filePath(QStringLiteral("test.pdf"));
    QVERIFY(DrawingExporter::exportDrawing(pdfPath, &doc, ExportOptions(), &error));

    // PNG export
    QString pngPath = tempDir.filePath(QStringLiteral("test.png"));
    QVERIFY(DrawingExporter::exportDrawing(pngPath, &doc, ExportOptions(), &error));

    // Reject DWG without claiming false support
    QString dwgPath = tempDir.filePath(QStringLiteral("test.dwg"));
    QVERIFY(!DrawingExporter::exportDrawing(dwgPath, &doc, ExportOptions(), &error));
    QVERIFY(error.contains(QStringLiteral("DWG"), Qt::CaseInsensitive));

    // Reject unsupported extension
    QString xyzPath = tempDir.filePath(QStringLiteral("test.xyz"));
    QVERIFY(!DrawingExporter::exportDrawing(xyzPath, &doc, ExportOptions(), &error));
}

void FileIoTests::testExportDoesNotModifyDocumentState() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    CadDocument doc;
    doc.setFilePath(QStringLiteral("my_native_project.gencad"));
    doc.setModified(true);
    doc.addEntity(std::make_shared<CircleEntity>(QPointF(10, 10), 5.0));

    QString error;
    QString dxfPath = tempDir.filePath(QStringLiteral("out.dxf"));
    QVERIFY(DrawingExporter::exportDrawing(dxfPath, &doc, ExportOptions(), &error));

    // Document state must be preserved
    QCOMPARE(doc.filePath(), QStringLiteral("my_native_project.gencad"));
    QVERIFY(doc.isModified());

    QString pdfPath = tempDir.filePath(QStringLiteral("out.pdf"));
    QVERIFY(DrawingExporter::exportDrawing(pdfPath, &doc, ExportOptions(), &error));
    QCOMPARE(doc.filePath(), QStringLiteral("my_native_project.gencad"));
    QVERIFY(doc.isModified());
}

#include "FileIoTests.moc"

QObject* createFileIoTests() {
    return new FileIoTests();
}
