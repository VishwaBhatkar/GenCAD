#include "io/DrawingExporter.h"
#include "io/DxfFile.h"
#include "document/CadDocument.h"
#include "document/PageSettings.h"
#include "document/UnitSystem.h"
#include "layers/LayerManager.h"
#include "layers/CadLayer.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"
#include "geometry/ArcEntity.h"
#include "geometry/PolylineEntity.h"
#include "geometry/PolygonEntity.h"
#include "geometry/EllipseEntity.h"
#include "geometry/TextEntity.h"
#include "geometry/DimensionEntity.h"

#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QFontMetricsF>
#include <QImage>
#include <QPdfWriter>
#include <QPageSize>
#include <QPageLayout>
#include <QFileInfo>
#include <QDir>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ExportFormat DrawingExporter::formatFromExtension(const QString& filePath) {
    QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == QStringLiteral("dxf")) return ExportFormat::Dxf;
    if (ext == QStringLiteral("pdf")) return ExportFormat::Pdf;
    if (ext == QStringLiteral("png")) return ExportFormat::Png;
    return ExportFormat::Unknown;
}

QString DrawingExporter::exportFilter() {
    return QStringLiteral("DXF Drawing (*.dxf);;PDF Document (*.pdf);;PNG Image (*.png)");
}

static void renderDimensionHelper(QPainter* painter, const DimensionEntity* dim, const QColor& strokeColor, double scale) {
    if (!dim) return;

    double penW = std::max(1.0 / scale, 1.2);
    QPen pen(strokeColor, penW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    QPointF p1 = dim->p1();
    QPointF p2 = dim->p2();
    QPointF d = dim->dimPoint();

    double dx = p2.x() - p1.x();
    double dy = p2.y() - p1.y();
    double len = std::hypot(dx, dy);

    if (dim->dimensionType() == DimensionType::Radius || dim->dimensionType() == DimensionType::Diameter) {
        painter->drawLine(p1, p2);

        if (len > 1e-4) {
            double ux = dx / len;
            double uy = dy / len;
            double tick = 5.0 / scale;
            painter->drawLine(p2, p2 - QPointF((ux - uy * 0.4) * tick, (uy + ux * 0.4) * tick));
            painter->drawLine(p2, p2 - QPointF((ux + uy * 0.4) * tick, (uy - ux * 0.4) * tick));
        }

        QPointF midPt = (p1 + p2) * 0.5;
        painter->save();
        painter->translate(midPt);
        QFont font = painter->font();
        font.setPointSizeF(std::max(8.0, 10.0 / scale));
        painter->setFont(font);
        QString txt = dim->dimensionText();
        QFontMetricsF fm(font);
        QRectF txtRect = fm.boundingRect(txt);
        painter->setPen(strokeColor);
        painter->drawText(QPointF(-txtRect.width() * 0.5, -4.0 / scale), txt);
        painter->restore();
    } else {
        if (len > 1e-4) {
            double ux = dx / len;
            double uy = dy / len;
            double nx = -uy;
            double ny = ux;

            double projOffset = (d.x() - p1.x()) * nx + (d.y() - p1.y()) * ny;
            QPointF dp1 = p1 + QPointF(nx * projOffset, ny * projOffset);
            QPointF dp2 = p2 + QPointF(nx * projOffset, ny * projOffset);

            // Witness extension lines
            QPen witnessPen(strokeColor, std::max(0.8 / scale, 0.8), Qt::SolidLine);
            painter->setPen(witnessPen);
            double witnessOvershoot = (projOffset >= 0 ? 3.0 : -3.0) / scale;
            painter->drawLine(p1, dp1 + QPointF(nx * witnessOvershoot, ny * witnessOvershoot));
            painter->drawLine(p2, dp2 + QPointF(nx * witnessOvershoot, ny * witnessOvershoot));

            // Dimension line
            painter->setPen(pen);
            painter->drawLine(dp1, dp2);

            // Arrowheads / tick marks
            double tick = 5.0 / scale;
            painter->drawLine(dp1 - QPointF((ux + nx) * tick, (uy + ny) * tick),
                              dp1 + QPointF((ux + nx) * tick, (uy + ny) * tick));
            painter->drawLine(dp2 - QPointF((ux + nx) * tick, (uy + ny) * tick),
                              dp2 + QPointF((ux + nx) * tick, (uy + ny) * tick));

            // Dimension measurement text
            QPointF midDim = (dp1 + dp2) * 0.5;
            painter->save();
            painter->translate(midDim);
            double angleDeg = std::atan2(dy, dx) * 180.0 / M_PI;
            if (angleDeg > 90.0 || angleDeg < -90.0) angleDeg += 180.0;
            painter->rotate(angleDeg);

            QFont font = painter->font();
            font.setPointSizeF(std::max(8.0, 10.0 / scale));
            painter->setFont(font);
            QString txt = dim->dimensionText();
            QFontMetricsF fm(font);
            QRectF txtRect = fm.boundingRect(txt);
            painter->setPen(strokeColor);
            painter->drawText(QPointF(-txtRect.width() * 0.5, -3.0 / scale), txt);
            painter->restore();
        }
    }
}

bool DrawingExporter::renderDrawing(QPainter* painter,
                                    const CadDocument* document,
                                    const QRectF& targetRect,
                                    const ExportOptions& options,
                                    QStringList* warnings)
{
    if (!painter || !painter->isActive() || !document) return false;
    if (document->entities().empty()) return false;

    QRectF bbox = document->boundingBox();
    if (bbox.isEmpty() || !bbox.isValid() || (bbox.width() < 1e-4 && bbox.height() < 1e-4)) {
        bbox = QRectF(0.0, 0.0, 100.0, 100.0);
    } else {
        if (bbox.width() < 1e-4) bbox.adjust(-10.0, 0.0, 10.0, 0.0);
        if (bbox.height() < 1e-4) bbox.adjust(0.0, -10.0, 0.0, 10.0);
    }

    // Add 6% margin around bounding box for clean presentation
    double margin = std::max(10.0, std::max(bbox.width(), bbox.height()) * 0.06);
    QRectF sourceRect = bbox.adjusted(-margin, -margin, margin, margin);

    double scaleX = targetRect.width() / sourceRect.width();
    double scaleY = targetRect.height() / sourceRect.height();
    double scale = std::min(scaleX, scaleY);
    if (scale <= 1e-9) scale = 1.0;

    double offX = targetRect.left() + (targetRect.width() - sourceRect.width() * scale) * 0.5 - sourceRect.left() * scale;
    double offY = targetRect.top() + (targetRect.height() - sourceRect.height() * scale) * 0.5 - sourceRect.top() * scale;

    if (options.drawBackground) {
        painter->fillRect(targetRect, options.backgroundColor);
    }

    painter->save();
    painter->translate(offX, offY);
    painter->scale(scale, scale);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    bool isLightBackground = (options.backgroundColor.lightness() > 128);

    for (const auto& entity : document->entities()) {
        if (!entity) continue;

        // Layer visibility check
        if (document->layerManager()) {
            auto layer = document->layerManager()->findLayer(entity->layer());
            if (layer && !layer->isVisible()) {
                continue;
            }
        }

        // Color resolution
        QColor strokeColor = entity->color();
        if (document->layerManager()) {
            auto layer = document->layerManager()->findLayer(entity->layer());
            if (layer && (strokeColor == Qt::white || strokeColor == Qt::black || !strokeColor.isValid())) {
                strokeColor = layer->color();
            }
        }

        // If background is light, adapt white or near-white strokes to black
        if (isLightBackground) {
            if (strokeColor == Qt::white || (strokeColor.red() > 240 && strokeColor.green() > 240 && strokeColor.blue() > 240)) {
                strokeColor = Qt::black;
            }
        }

        double strokeW = std::max(1.0 / scale, entity->strokeWidth());
        QPen pen(strokeColor, strokeW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);

        switch (entity->type()) {
            case EntityType::Line: {
                const auto* line = static_cast<const LineEntity*>(entity.get());
                painter->drawLine(line->p1(), line->p2());
                break;
            }
            case EntityType::Circle: {
                const auto* circle = static_cast<const CircleEntity*>(entity.get());
                painter->drawEllipse(circle->center(), circle->radius(), circle->radius());
                break;
            }
            case EntityType::Rectangle: {
                const auto* rect = static_cast<const RectangleEntity*>(entity.get());
                const auto& c = rect->corners();
                painter->drawPolygon(c.data(), 4);
                break;
            }
            case EntityType::Arc: {
                const auto* arc = static_cast<const ArcEntity*>(entity.get());
                QRectF arcRect(arc->center().x() - arc->radius(),
                               arc->center().y() - arc->radius(),
                               2.0 * arc->radius(),
                               2.0 * arc->radius());
                painter->drawArc(arcRect,
                                static_cast<int>(arc->startAngle() * 16.0),
                                static_cast<int>(arc->sweepAngle() * 16.0));
                break;
            }
            case EntityType::Polyline: {
                const auto* poly = static_cast<const PolylineEntity*>(entity.get());
                const auto& pts = poly->points();
                if (pts.size() >= 2) {
                    if (poly->isClosed()) {
                        painter->drawPolygon(pts.data(), static_cast<int>(pts.size()));
                    } else {
                        painter->drawPolyline(pts.data(), static_cast<int>(pts.size()));
                    }
                }
                break;
            }
            case EntityType::Polygon: {
                const auto* poly = static_cast<const PolygonEntity*>(entity.get());
                auto pts = poly->vertices();
                if (pts.size() >= 3) {
                    painter->drawPolygon(pts.data(), static_cast<int>(pts.size()));
                }
                break;
            }
            case EntityType::Ellipse: {
                const auto* ellipse = static_cast<const EllipseEntity*>(entity.get());
                painter->save();
                painter->translate(ellipse->center());
                painter->rotate(ellipse->rotationDeg());
                painter->drawEllipse(QPointF(0, 0), ellipse->radiusX(), ellipse->radiusY());
                painter->restore();
                break;
            }
            case EntityType::Text: {
                const auto* text = static_cast<const TextEntity*>(entity.get());
                painter->save();
                painter->translate(text->position());
                painter->rotate(text->rotationDeg());

                QFont font = painter->font();
                font.setPixelSize(static_cast<int>(std::max(2.0, text->height())));
                painter->setFont(font);
                painter->setPen(strokeColor);
                painter->drawText(QPointF(0, 0), text->text());
                painter->restore();
                break;
            }
            case EntityType::Dimension: {
                const auto* dim = static_cast<const DimensionEntity*>(entity.get());
                renderDimensionHelper(painter, dim, strokeColor, scale);
                break;
            }
            default: {
                if (warnings) {
                    QString typeStr = CadEntity::entityTypeToString(entity->type());
                    if (!warnings->contains(typeStr)) {
                        warnings->append(typeStr);
                    }
                }
                break;
            }
        }
    }

    painter->restore();
    return true;
}

bool DrawingExporter::exportDxf(const QString& filePath,
                                const CadDocument* document,
                                QString* errorMessage,
                                QStringList* warnings)
{
    return DxfFile::save(filePath, document, errorMessage, warnings);
}

bool DrawingExporter::exportPdf(const QString& filePath,
                                const CadDocument* document,
                                const ExportOptions& options,
                                QString* errorMessage,
                                QStringList* warnings)
{
    if (!document) {
        if (errorMessage) *errorMessage = QStringLiteral("Invalid document pointer");
        return false;
    }

    if (filePath.trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("File path cannot be empty");
        return false;
    }

    if (document->entities().empty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Cannot export an empty drawing");
        return false;
    }

    // Verify parent directory exists or create it
    QFileInfo fi(filePath);
    QDir dir = fi.dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (errorMessage) *errorMessage = QStringLiteral("Cannot create directory: %1").arg(dir.absolutePath());
        return false;
    }

    QPdfWriter writer(filePath);

    // Apply document page settings
    const auto& pageSettings = document->pageSettings();
    if (pageSettings.pageSize() == PageSize::Custom) {
        writer.setPageSize(QPageSize(QSizeF(pageSettings.customWidth(), pageSettings.customHeight()), QPageSize::Millimeter));
    } else {
        QPageSize::PageSizeId psId = QPageSize::A4;
        switch (pageSettings.pageSize()) {
            case PageSize::A0: psId = QPageSize::A0; break;
            case PageSize::A1: psId = QPageSize::A1; break;
            case PageSize::A2: psId = QPageSize::A2; break;
            case PageSize::A3: psId = QPageSize::A3; break;
            case PageSize::A4: psId = QPageSize::A4; break;
            case PageSize::A5: psId = QPageSize::A5; break;
            default: psId = QPageSize::A4; break;
        }
        writer.setPageSize(QPageSize(psId));
    }

    writer.setPageOrientation(pageSettings.orientation() == PageOrientation::Landscape ? QPageLayout::Landscape : QPageLayout::Portrait);
    writer.setResolution(options.resolutionDpi > 0 ? options.resolutionDpi : 300);
    writer.setPageMargins(QMarginsF(10, 10, 10, 10), QPageLayout::Millimeter);

    QRectF paintRect = writer.pageLayout().paintRectPixels(writer.resolution());
    if (paintRect.width() <= 0 || paintRect.height() <= 0) {
        paintRect = QRectF(0, 0, 2480, 3508);
    }

    QPainter painter(&writer);
    if (!painter.isActive()) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to open PDF device for writing: %1").arg(filePath);
        return false;
    }

    ExportOptions pdfOptions = options;
    pdfOptions.backgroundColor = Qt::white;
    pdfOptions.drawBackground = true;

    bool ok = renderDrawing(&painter, document, paintRect, pdfOptions, warnings);
    painter.end();

    QFileInfo outInfo(filePath);
    if (!ok || !outInfo.exists() || outInfo.size() == 0) {
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = QStringLiteral("PDF export failed to create valid file: %1").arg(filePath);
        }
        return false;
    }

    return true;
}

bool DrawingExporter::exportPng(const QString& filePath,
                                const CadDocument* document,
                                const ExportOptions& options,
                                QString* errorMessage,
                                QStringList* warnings)
{
    if (!document) {
        if (errorMessage) *errorMessage = QStringLiteral("Invalid document pointer");
        return false;
    }

    if (filePath.trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("File path cannot be empty");
        return false;
    }

    if (document->entities().empty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Cannot export an empty drawing");
        return false;
    }

    QFileInfo fi(filePath);
    QDir dir = fi.dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (errorMessage) *errorMessage = QStringLiteral("Cannot create directory: %1").arg(dir.absolutePath());
        return false;
    }

    QRectF bbox = document->boundingBox();
    if (bbox.isEmpty() || !bbox.isValid() || (bbox.width() < 1e-4 && bbox.height() < 1e-4)) {
        bbox = QRectF(0.0, 0.0, 100.0, 100.0);
    } else {
        if (bbox.width() < 1e-4) bbox.adjust(-10.0, 0.0, 10.0, 0.0);
        if (bbox.height() < 1e-4) bbox.adjust(0.0, -10.0, 0.0, 10.0);
    }

    int imgW = 2400;
    int imgH = 1600;

    if (options.autoFitAspectRatio) {
        double aspect = bbox.width() / bbox.height();
        if (aspect >= 1.0) {
            imgW = 2400;
            imgH = static_cast<int>(2400.0 / aspect);
            imgH = std::clamp(imgH, 400, 3200);
        } else {
            imgH = 2400;
            imgW = static_cast<int>(2400.0 * aspect);
            imgW = std::clamp(imgW, 400, 3200);
        }
    } else if (options.imageSize.isValid()) {
        imgW = options.imageSize.width();
        imgH = options.imageSize.height();
    }

    QImage image(imgW, imgH, QImage::Format_ARGB32_Premultiplied);
    image.fill(options.backgroundColor);

    QPainter painter(&image);
    if (!painter.isActive()) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to initialize PNG drawing surface");
        return false;
    }

    bool ok = renderDrawing(&painter, document, QRectF(0, 0, imgW, imgH), options, warnings);
    painter.end();

    if (!ok) {
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = QStringLiteral("Failed to render geometry to image");
        }
        return false;
    }

    if (!image.save(filePath, "PNG")) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to write PNG file: %1").arg(filePath);
        return false;
    }

    return true;
}

bool DrawingExporter::exportDrawing(const QString& filePath,
                                    const CadDocument* document,
                                    const ExportOptions& options,
                                    QString* errorMessage,
                                    QStringList* warnings)
{
    if (filePath.trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Export file path cannot be empty");
        return false;
    }

    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();

    if (ext == QStringLiteral("dwg")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("DWG export is not supported. Please export to standard DXF format.");
        }
        return false;
    }

    if (ext == QStringLiteral("dxf")) {
        return exportDxf(filePath, document, errorMessage, warnings);
    } else if (ext == QStringLiteral("pdf")) {
        return exportPdf(filePath, document, options, errorMessage, warnings);
    } else if (ext == QStringLiteral("png")) {
        return exportPng(filePath, document, options, errorMessage, warnings);
    } else {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unsupported export format: .%1. Supported formats are .dxf, .pdf, and .png").arg(ext);
        }
        return false;
    }
}
