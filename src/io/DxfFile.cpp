#include "io/DxfFile.h"
#include "document/CadDocument.h"
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
#include "document/UnitSystem.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <cmath>
#include <vector>

int DxfFile::rgbToAci(const QColor& color) {
    if (!color.isValid()) return 7; // Default White/Black

    int r = color.red();
    int g = color.green();
    int b = color.blue();

    // Standard AutoCAD Color Index (ACI) matching
    if (r > 200 && g < 50 && b < 50) return 1;    // Red
    if (r > 200 && g > 200 && b < 50) return 2;  // Yellow
    if (r < 50 && g > 200 && b < 50) return 3;   // Green
    if (r < 50 && g > 200 && b > 200) return 4;  // Cyan
    if (r < 50 && g < 50 && b > 200) return 5;   // Blue
    if (r > 200 && g < 50 && b > 200) return 6;  // Magenta
    if (r > 200 && g > 200 && b > 200) return 7; // White

    // Euclidean distance fallback to the 7 basic colors
    const struct { int aci; int r; int g; int b; } basicColors[] = {
        {1, 255, 0, 0},
        {2, 255, 255, 0},
        {3, 0, 255, 0},
        {4, 0, 255, 255},
        {5, 0, 0, 255},
        {6, 255, 0, 255},
        {7, 255, 255, 255},
        {8, 128, 128, 128}
    };

    int bestAci = 7;
    int minDiff = 10000000;
    for (const auto& bc : basicColors) {
        int diff = (r - bc.r) * (r - bc.r) + (g - bc.g) * (g - bc.g) + (b - bc.b) * (b - bc.b);
        if (diff < minDiff) {
            minDiff = diff;
            bestAci = bc.aci;
        }
    }
    return bestAci;
}

QColor DxfFile::aciToRgb(int aci) {
    switch (std::abs(aci)) {
        case 1: return QColor(255, 0, 0);     // Red
        case 2: return QColor(255, 255, 0);   // Yellow
        case 3: return QColor(0, 255, 0);     // Green
        case 4: return QColor(0, 255, 255);   // Cyan
        case 5: return QColor(0, 0, 255);     // Blue
        case 6: return QColor(255, 0, 255);   // Magenta
        case 7: return QColor(255, 255, 255); // White
        case 8: return QColor(128, 128, 128); // Dark Grey
        case 9: return QColor(192, 192, 192); // Light Grey
        default: return QColor(255, 255, 255);
    }
}

bool DxfFile::save(const QString& filePath, const CadDocument* document, QString* errorMessage, QStringList* warnings) {
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

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage) *errorMessage = file.errorString();
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(6);

    auto writePair = [&out](int code, const QString& val) {
        out << "  " << code << "\n" << val << "\n";
    };
    auto writeNum = [&out](int code, double val) {
        out << "  " << code << "\n" << QString::number(val, 'f', 6) << "\n";
    };

    // 1. HEADER SECTION
    writePair(0, "SECTION");
    writePair(2, "HEADER");
    writePair(9, "$ACADVER");
    writePair(1, "AC1015"); // AutoCAD 2000 ASCII DXF standard for LWPOLYLINE and ELLIPSE

    // Write Units
    writePair(9, "$INSUNITS");
    int insunits = 4; // Millimeter default
    switch (document->unitSettings().type()) {
        case UnitType::Millimeter: insunits = 4; break;
        case UnitType::Centimeter: insunits = 5; break;
        case UnitType::Meter: insunits = 6; break;
        case UnitType::Inch: insunits = 1; break;
        case UnitType::Foot: insunits = 2; break;
        default: insunits = 0; break;
    }
    writePair(70, QString::number(insunits));

    QRectF bbox = document->boundingBox();
    if (bbox.isEmpty() || !bbox.isValid()) {
        bbox = QRectF(0.0, 0.0, 100.0, 100.0);
    }
    writePair(9, "$EXTMIN");
    writeNum(10, bbox.left());
    writeNum(20, bbox.top());
    writeNum(30, 0.0);
    writePair(9, "$EXTMAX");
    writeNum(10, bbox.right());
    writeNum(20, bbox.bottom());
    writeNum(30, 0.0);
    writePair(0, "ENDSEC");

    // 2. TABLES SECTION (LAYERS)
    writePair(0, "SECTION");
    writePair(2, "TABLES");
    writePair(0, "TABLE");
    writePair(2, "LAYER");

    if (document->layerManager()) {
        const auto& layers = document->layerManager()->layers();
        writePair(70, QString::number(layers.size()));
        for (const auto& layer : layers) {
            writePair(0, "LAYER");
            writePair(2, layer->name());
            int flags = 0;
            if (layer->isLocked()) flags |= 4;
            writePair(70, QString::number(flags));

            int aci = rgbToAci(layer->color());
            if (!layer->isVisible()) aci = -std::abs(aci); // Negative ACI means layer is turned OFF in DXF
            writePair(62, QString::number(aci));
            writePair(6, "CONTINUOUS");
        }
    } else {
        writePair(70, "1");
        writePair(0, "LAYER");
        writePair(2, "0");
        writePair(70, "0");
        writePair(62, "7");
        writePair(6, "CONTINUOUS");
    }
    writePair(0, "ENDTAB");
    writePair(0, "ENDSEC");

    // 3. ENTITIES SECTION
    writePair(0, "SECTION");
    writePair(2, "ENTITIES");

    for (const auto& entity : document->entities()) {
        if (!entity) continue;
        QString layerName = entity->layer().isEmpty() ? QStringLiteral("0") : entity->layer();
        int aci = rgbToAci(entity->color());
        if (document->layerManager()) {
            auto layer = document->layerManager()->findLayer(entity->layer());
            if (layer && (entity->color() == Qt::white || entity->color() == Qt::black || !entity->color().isValid())) {
                aci = rgbToAci(layer->color());
            }
        }

        switch (entity->type()) {
            case EntityType::Line: {
                const auto* line = static_cast<const LineEntity*>(entity.get());
                writePair(0, "LINE");
                writePair(8, layerName);
                writePair(62, QString::number(aci));
                writeNum(10, line->p1().x());
                writeNum(20, line->p1().y());
                writeNum(30, 0.0);
                writeNum(11, line->p2().x());
                writeNum(21, line->p2().y());
                writeNum(31, 0.0);
                break;
            }
            case EntityType::Circle: {
                const auto* circle = static_cast<const CircleEntity*>(entity.get());
                writePair(0, "CIRCLE");
                writePair(8, layerName);
                writePair(62, QString::number(aci));
                writeNum(10, circle->center().x());
                writeNum(20, circle->center().y());
                writeNum(30, 0.0);
                writeNum(40, circle->radius());
                break;
            }
            case EntityType::Rectangle: {
                const auto* rect = static_cast<const RectangleEntity*>(entity.get());
                const auto& c = rect->corners();
                writePair(0, "LWPOLYLINE");
                writePair(8, layerName);
                writePair(62, QString::number(aci));
                writePair(90, "4");
                writePair(70, "1"); // Closed
                for (size_t k = 0; k < 4; ++k) {
                    writeNum(10, c[k].x());
                    writeNum(20, c[k].y());
                }
                break;
            }
            case EntityType::Arc: {
                const auto* arc = static_cast<const ArcEntity*>(entity.get());
                writePair(0, "ARC");
                writePair(8, layerName);
                writePair(62, QString::number(aci));
                writeNum(10, arc->center().x());
                writeNum(20, arc->center().y());
                writeNum(30, 0.0);
                writeNum(40, arc->radius());
                double sAngle = std::fmod(arc->startAngle(), 360.0);
                if (sAngle < 0.0) sAngle += 360.0;
                double eAngle = std::fmod(arc->endAngle(), 360.0);
                if (eAngle < 0.0) eAngle += 360.0;
                writeNum(50, sAngle);
                writeNum(51, eAngle);
                break;
            }
            case EntityType::Polyline: {
                const auto* poly = static_cast<const PolylineEntity*>(entity.get());
                const auto& pts = poly->points();
                if (pts.size() >= 2) {
                    writePair(0, "LWPOLYLINE");
                    writePair(8, layerName);
                    writePair(62, QString::number(aci));
                    writePair(90, QString::number(pts.size()));
                    writePair(70, poly->isClosed() ? "1" : "0");
                    for (const auto& pt : pts) {
                        writeNum(10, pt.x());
                        writeNum(20, pt.y());
                    }
                }
                break;
            }
            case EntityType::Polygon: {
                const auto* poly = static_cast<const PolygonEntity*>(entity.get());
                auto pts = poly->vertices();
                if (pts.size() >= 3) {
                    writePair(0, "LWPOLYLINE");
                    writePair(8, layerName);
                    writePair(62, QString::number(aci));
                    writePair(90, QString::number(pts.size()));
                    writePair(70, "1"); // Closed
                    for (const auto& pt : pts) {
                        writeNum(10, pt.x());
                        writeNum(20, pt.y());
                    }
                }
                break;
            }
            case EntityType::Ellipse: {
                const auto* ellipse = static_cast<const EllipseEntity*>(entity.get());
                writePair(0, "ELLIPSE");
                writePair(8, layerName);
                writePair(62, QString::number(aci));
                writeNum(10, ellipse->center().x());
                writeNum(20, ellipse->center().y());
                writeNum(30, 0.0);

                double rx = ellipse->radiusX();
                double ry = ellipse->radiusY();
                double rotRad = ellipse->rotationDeg() * 3.14159265358979323846 / 180.0;
                double majorR = rx;
                double minorR = ry;
                double majorAngle = rotRad;

                if (ry > rx && rx > 1e-9) {
                    majorR = ry;
                    minorR = rx;
                    majorAngle = rotRad + 3.14159265358979323846 / 2.0;
                }

                double ratio = (majorR > 1e-9) ? (minorR / majorR) : 1.0;
                double endMajorX = majorR * std::cos(majorAngle);
                double endMajorY = majorR * std::sin(majorAngle);

                writeNum(11, endMajorX);
                writeNum(21, endMajorY);
                writeNum(31, 0.0);
                writeNum(40, ratio);
                writeNum(41, 0.0);
                writeNum(42, 2.0 * 3.14159265358979323846);
                break;
            }
            case EntityType::Text: {
                const auto* text = static_cast<const TextEntity*>(entity.get());
                writePair(0, "TEXT");
                writePair(8, layerName);
                writePair(62, QString::number(aci));
                writeNum(10, text->position().x());
                writeNum(20, text->position().y());
                writeNum(30, 0.0);
                writeNum(40, text->height());
                writePair(1, text->text());
                writeNum(50, text->rotationDeg());
                break;
            }
            case EntityType::Dimension: {
                const auto* dim = static_cast<const DimensionEntity*>(entity.get());
                writePair(0, "DIMENSION");
                writePair(8, layerName);
                writePair(62, QString::number(aci));
                writeNum(10, dim->dimPoint().x());
                writeNum(20, dim->dimPoint().y());
                writeNum(30, 0.0);
                writeNum(13, dim->p1().x());
                writeNum(23, dim->p1().y());
                writeNum(33, 0.0);
                writeNum(14, dim->p2().x());
                writeNum(24, dim->p2().y());
                writeNum(34, 0.0);
                writePair(70, "0"); // Linear dimension
                writePair(1, dim->dimensionText());
                break;
            }
            default: {
                if (warnings) {
                    QString typeName = CadEntity::entityTypeToString(entity->type());
                    if (!warnings->contains(typeName)) {
                        warnings->append(typeName);
                    }
                }
                break;
            }
        }
    }

    writePair(0, "ENDSEC");
    writePair(0, "EOF");

    file.close();
    return true;
}

bool DxfFile::load(const QString& filePath, CadDocument* document, QString* errorMessage, QStringList* warnings) {
    if (!document) {
        if (errorMessage) *errorMessage = QStringLiteral("Invalid document pointer");
        return false;
    }

    if (filePath.trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("File path cannot be empty");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = file.errorString();
        return false;
    }

    struct DxfRecord {
        int code;
        QString value;
    };

    std::vector<DxfRecord> records;
    records.reserve(8192);

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString codeLine = in.readLine().trimmed();
        if (codeLine.isEmpty() && in.atEnd()) break;
        QString valLine = in.readLine().trimmed();

        bool ok = false;
        int code = codeLine.toInt(&ok);
        if (ok) {
            records.push_back({code, valLine});
        }
    }
    file.close();

    if (records.empty()) {
        if (errorMessage) *errorMessage = QStringLiteral("DXF file is empty");
        return false;
    }

    document->clear();

    size_t i = 0;
    while (i < records.size()) {
        if (records[i].code == 0 && records[i].value == QStringLiteral("SECTION")) {
            ++i;
            if (i >= records.size()) break;
            QString sectionType;
            if (records[i].code == 2) {
                sectionType = records[i].value;
                ++i;
            }

            if (sectionType == QStringLiteral("HEADER")) {
                while (i < records.size()) {
                    if (records[i].code == 0 && records[i].value == QStringLiteral("ENDSEC")) {
                        ++i;
                        break;
                    }
                    if (records[i].code == 9 && records[i].value == QStringLiteral("$INSUNITS")) {
                        ++i;
                        if (i < records.size() && records[i].code == 70) {
                            int uCode = records[i].value.toInt();
                            UnitType uType = UnitType::Millimeter;
                            switch (uCode) {
                                case 1: uType = UnitType::Inch; break;
                                case 2: uType = UnitType::Foot; break;
                                case 4: uType = UnitType::Millimeter; break;
                                case 5: uType = UnitType::Centimeter; break;
                                case 6: uType = UnitType::Meter; break;
                                default: uType = UnitType::Millimeter; break;
                            }
                            document->unitSettings().setType(uType);
                        }
                    } else {
                        ++i;
                    }
                }
            } else if (sectionType == QStringLiteral("TABLES")) {
                // Parse tables (especially LAYER)
                while (i < records.size()) {
                    if (records[i].code == 0 && records[i].value == QStringLiteral("ENDSEC")) {
                        ++i;
                        break;
                    }

                    if (records[i].code == 0 && records[i].value == QStringLiteral("LAYER")) {
                        ++i;
                        QString layerName;
                        int aci = 7;
                        bool locked = false;

                        while (i < records.size() && records[i].code != 0) {
                            if (records[i].code == 2) {
                                layerName = records[i].value;
                            } else if (records[i].code == 62) {
                                aci = records[i].value.toInt();
                            } else if (records[i].code == 70) {
                                int flags = records[i].value.toInt();
                                if (flags & 4) locked = true;
                            }
                            ++i;
                        }

                        if (!layerName.isEmpty() && document->layerManager()) {
                            QColor col = aciToRgb(aci);
                            document->layerManager()->addLayer(layerName, col);
                            auto l = document->layerManager()->findLayer(layerName);
                            if (l) {
                                if (aci < 0) l->setVisible(false);
                                if (locked) l->setLocked(true);
                            }
                        }
                    } else {
                        ++i;
                    }
                }
            } else if (sectionType == QStringLiteral("ENTITIES")) {
                // Parse entities
                while (i < records.size()) {
                    if (records[i].code == 0 && records[i].value == QStringLiteral("ENDSEC")) {
                        ++i;
                        break;
                    }

                    if (records[i].code == 0) {
                        QString entityType = records[i].value;
                        ++i;

                        QString layer = QStringLiteral("0");
                        int aci = 7;
                        double x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0;
                        double radius = 0.0, startAngle = 0.0, endAngle = 0.0, height = 10.0, rot = 0.0;
                        QString textVal;
                        std::vector<QPointF> polyPts;
                        bool isClosed = false;

                        while (i < records.size() && records[i].code != 0) {
                            int c = records[i].code;
                            const QString& v = records[i].value;

                            if (c == 8) layer = v;
                            else if (c == 62) aci = v.toInt();
                            else if (c == 10) x1 = v.toDouble();
                            else if (c == 20) y1 = v.toDouble();
                            else if (c == 11) x2 = v.toDouble();
                            else if (c == 21) y2 = v.toDouble();
                            else if (c == 13) x2 = v.toDouble(); // For DIMENSION p1
                            else if (c == 23) y2 = v.toDouble();
                            else if (c == 14) x3 = v.toDouble(); // For DIMENSION p2
                            else if (c == 24) y3 = v.toDouble();
                            else if (c == 40) {
                                radius = v.toDouble();
                                height = v.toDouble();
                            }
                            else if (c == 50) {
                                startAngle = v.toDouble();
                                rot = v.toDouble();
                            }
                            else if (c == 51) endAngle = v.toDouble();
                            else if (c == 1) textVal = v;
                            else if (c == 70) isClosed = (v.toInt() & 1);

                            if (entityType == QStringLiteral("LWPOLYLINE") && c == 10) {
                                double px = v.toDouble();
                                double py = 0.0;
                                if (i + 1 < records.size() && records[i + 1].code == 20) {
                                    py = records[i + 1].value.toDouble();
                                    ++i;
                                }
                                polyPts.push_back(QPointF(px, py));
                            }

                            ++i;
                        }

                        std::shared_ptr<CadEntity> entity;
                        if (entityType == QStringLiteral("LINE")) {
                            entity = std::make_shared<LineEntity>(QPointF(x1, y1), QPointF(x2, y2));
                        } else if (entityType == QStringLiteral("CIRCLE")) {
                            entity = std::make_shared<CircleEntity>(QPointF(x1, y1), radius);
                        } else if (entityType == QStringLiteral("ARC")) {
                            double sweep = endAngle - startAngle;
                            while (sweep <= 0.0) sweep += 360.0;
                            while (sweep > 360.0) sweep -= 360.0;
                            entity = std::make_shared<ArcEntity>(QPointF(x1, y1), radius, startAngle, sweep);
                        } else if (entityType == QStringLiteral("LWPOLYLINE")) {
                            if (!polyPts.empty()) {
                                entity = std::make_shared<PolylineEntity>(polyPts, isClosed);
                            }
                        } else if (entityType == QStringLiteral("POLYLINE")) {
                            std::vector<QPointF> polylinePts;
                            bool closed = isClosed;
                            while (i < records.size()) {
                                if (records[i].code == 0) {
                                    if (records[i].value == QStringLiteral("SEQEND") || records[i].value == QStringLiteral("ENDSEC")) {
                                        if (records[i].value == QStringLiteral("SEQEND")) ++i;
                                        break;
                                    }
                                    if (records[i].value == QStringLiteral("VERTEX")) {
                                        ++i;
                                        double vx = 0, vy = 0;
                                        while (i < records.size() && records[i].code != 0) {
                                            if (records[i].code == 10) vx = records[i].value.toDouble();
                                            else if (records[i].code == 20) vy = records[i].value.toDouble();
                                            ++i;
                                        }
                                        polylinePts.push_back(QPointF(vx, vy));
                                        continue;
                                    }
                                }
                                ++i;
                            }
                            if (polylinePts.size() >= 2) {
                                entity = std::make_shared<PolylineEntity>(polylinePts, closed);
                            }
                        } else if (entityType == QStringLiteral("ELLIPSE")) {
                            double majorR = std::hypot(x2, y2);
                            if (majorR < 1e-6) majorR = 10.0;
                            double ratioVal = (radius > 1e-9 && radius <= 1.0) ? radius : 0.5;
                            double minorR = majorR * ratioVal;
                            double rotDeg = std::atan2(y2, x2) * 180.0 / 3.14159265358979323846;
                            entity = std::make_shared<EllipseEntity>(QPointF(x1, y1), majorR, minorR, rotDeg);
                        } else if (entityType == QStringLiteral("TEXT")) {
                            entity = std::make_shared<TextEntity>(QPointF(x1, y1), textVal, height, rot);
                        } else if (entityType == QStringLiteral("DIMENSION")) {
                            entity = std::make_shared<DimensionEntity>(QPointF(x2, y2), QPointF(x3, y3), QPointF(x1, y1));
                        } else {
                            if (warnings && !warnings->contains(entityType)) {
                                warnings->append(QStringLiteral("Unsupported DXF entity: %1").arg(entityType));
                            }
                        }

                        if (entity) {
                            entity->setLayer(layer);
                            if (aci != 7) {
                                entity->setColor(aciToRgb(aci));
                            }
                            document->addEntity(entity);
                        }
                    } else {
                        ++i;
                    }
                }
            } else {
                // Skip other sections
                while (i < records.size()) {
                    if (records[i].code == 0 && records[i].value == QStringLiteral("ENDSEC")) {
                        ++i;
                        break;
                    }
                    ++i;
                }
            }
        } else {
            ++i;
        }
    }

    // Never set filePath to the DXF file path to avoid accidentally saving JSON over DXF
    document->setFilePath(QString());
    document->setModified(false);
    return true;
}
