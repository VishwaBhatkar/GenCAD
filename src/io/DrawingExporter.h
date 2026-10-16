#pragma once

#include <QString>
#include <QStringList>
#include <QColor>
#include <QSize>
#include <QRectF>

class CadDocument;
class QPainter;

enum class ExportFormat {
    Dxf,
    Pdf,
    Png,
    Unknown
};

struct ExportOptions {
    ExportFormat format{ExportFormat::Dxf};
    QColor backgroundColor{Qt::white};
    bool drawBackground{true};
    int resolutionDpi{300};       // 300 DPI for vector PDF and PNG exports
    QSize imageSize{2400, 1600};   // Base size for PNG export
    bool autoFitAspectRatio{true}; // Adjust PNG dimensions to match drawing aspect ratio
};

class DrawingExporter {
public:
    static bool exportDrawing(const QString& filePath,
                             const CadDocument* document,
                             const ExportOptions& options = ExportOptions(),
                             QString* errorMessage = nullptr,
                             QStringList* warnings = nullptr);

    static bool exportDxf(const QString& filePath,
                          const CadDocument* document,
                          QString* errorMessage = nullptr,
                          QStringList* warnings = nullptr);

    static bool exportPdf(const QString& filePath,
                          const CadDocument* document,
                          const ExportOptions& options = ExportOptions(),
                          QString* errorMessage = nullptr,
                          QStringList* warnings = nullptr);

    static bool exportPng(const QString& filePath,
                          const CadDocument* document,
                          const ExportOptions& options = ExportOptions(),
                          QString* errorMessage = nullptr,
                          QStringList* warnings = nullptr);

    // Renders actual document geometry directly to any QPainter (image, pdf, etc.)
    static bool renderDrawing(QPainter* painter,
                              const CadDocument* document,
                              const QRectF& targetRect,
                              const ExportOptions& options = ExportOptions(),
                              QStringList* warnings = nullptr);

    static ExportFormat formatFromExtension(const QString& filePath);
    static QString exportFilter();
};
