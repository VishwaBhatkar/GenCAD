#include "ai/AiGeometryAnalyzer.h"
#include "document/CadDocument.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"
#include "geometry/ArcEntity.h"
#include "geometry/PolylineEntity.h"
#include "geometry/TextEntity.h"
#include "geometry/DimensionEntity.h"
#include <cmath>
#include <set>

bool AiGeometryAnalyzer::arePointsEqual(const QPointF& p1, const QPointF& p2, double tol) {
    return std::hypot(p1.x() - p2.x(), p1.y() - p2.y()) <= tol;
}

bool AiGeometryAnalyzer::areLinesColinearAndOverlapping(const LineEntity* l1, const LineEntity* l2, double tol) {
    if (!l1 || !l2) return false;
    QPointF p1 = l1->startPoint();
    QPointF p2 = l1->endPoint();
    QPointF q1 = l2->startPoint();
    QPointF q2 = l2->endPoint();

    double dx1 = p2.x() - p1.x();
    double dy1 = p2.y() - p1.y();
    double len1 = std::hypot(dx1, dy1);
    if (len1 < tol) return false;

    double dx2 = q2.x() - q1.x();
    double dy2 = q2.y() - q1.y();
    double len2 = std::hypot(dx2, dy2);
    if (len2 < tol) return false;

    // Unit direction of line 1
    double ux1 = dx1 / len1;
    double uy1 = dy1 / len1;

    // Unit normal of line 1
    double nx1 = -uy1;
    double ny1 = ux1;

    // Distance of q1 and q2 from line 1
    double d1 = std::abs((q1.x() - p1.x()) * nx1 + (q1.y() - p1.y()) * ny1);
    double d2 = std::abs((q2.x() - p1.x()) * nx1 + (q2.y() - p1.y()) * ny1);
    if (d1 > tol || d2 > tol) return false;

    // Project points onto line 1 axis
    double t_p1 = 0.0;
    double t_p2 = len1;
    double t_q1 = (q1.x() - p1.x()) * ux1 + (q1.y() - p1.y()) * uy1;
    double t_q2 = (q2.x() - p1.x()) * ux1 + (q2.y() - p1.y()) * uy1;

    double q_min = std::min(t_q1, t_q2);
    double q_max = std::max(t_q1, t_q2);

    // Overlap on 1D interval [0, len1] and [q_min, q_max]
    double overlap_start = std::max(0.0, q_min);
    double overlap_end = std::min(len1, q_max);

    return (overlap_end - overlap_start) > tol;
}

AiAnalysisReport AiGeometryAnalyzer::analyze(const CadDocument* doc, double tolerance) {
    AiAnalysisReport report;
    if (!doc) return report;

    const auto& entities = doc->entities();
    report.totalEntities = static_cast<int>(entities.size());
    report.boundingBox = doc->boundingBox();

    int dimensionCount = 0;
    int geometryCount = 0;
    std::vector<std::shared_ptr<LineEntity>> lines;

    // 1. Gather stats and categorize
    for (const auto& ent : entities) {
        if (!ent) continue;
        QString typeStr = CadEntity::entityTypeToString(ent->type());
        report.entityCountsByType[typeStr]++;

        switch (ent->type()) {
            case EntityType::Line: {
                auto l = std::dynamic_pointer_cast<LineEntity>(ent);
                if (l) {
                    lines.push_back(l);
                    report.totalWireLength += l->length();
                }
                geometryCount++;
                break;
            }
            case EntityType::Circle: {
                auto c = std::dynamic_pointer_cast<CircleEntity>(ent);
                if (c) {
                    report.totalWireLength += c->circumference();
                }
                geometryCount++;
                break;
            }
            case EntityType::Rectangle: {
                auto r = std::dynamic_pointer_cast<RectangleEntity>(ent);
                if (r) {
                    report.totalWireLength += r->perimeter();
                }
                geometryCount++;
                break;
            }
            case EntityType::Arc: {
                auto a = std::dynamic_pointer_cast<ArcEntity>(ent);
                if (a) {
                    report.totalWireLength += a->arcLength();
                }
                geometryCount++;
                break;
            }
            case EntityType::Polyline: {
                auto p = std::dynamic_pointer_cast<PolylineEntity>(ent);
                if (p) {
                    report.totalWireLength += p->totalLength();
                }
                geometryCount++;
                break;
            }
            case EntityType::Dimension: {
                dimensionCount++;
                break;
            }
            default:
                break;
        }

        // Layer check
        if (ent->layer() == QStringLiteral("0")) {
            report.layerIssues.push_back({
                ent->id(),
                ent->layer(),
                QStringLiteral("Entity '%1' is on default Layer '0'").arg(typeStr)
            });
        }
    }

    // 2. Duplicate Detection
    std::set<QUuid> markedDuplicates;
    for (size_t i = 0; i < entities.size(); ++i) {
        auto e1 = entities[i];
        if (!e1 || markedDuplicates.count(e1->id())) continue;

        for (size_t j = i + 1; j < entities.size(); ++j) {
            auto e2 = entities[j];
            if (!e2 || markedDuplicates.count(e2->id())) continue;
            if (e1->type() != e2->type()) continue;

            bool isDup = false;
            switch (e1->type()) {
                case EntityType::Line: {
                    auto l1 = std::dynamic_pointer_cast<LineEntity>(e1);
                    auto l2 = std::dynamic_pointer_cast<LineEntity>(e2);
                    if (l1 && l2) {
                        bool directMatch = arePointsEqual(l1->startPoint(), l2->startPoint(), tolerance) &&
                                          arePointsEqual(l1->endPoint(), l2->endPoint(), tolerance);
                        bool reverseMatch = arePointsEqual(l1->startPoint(), l2->endPoint(), tolerance) &&
                                           arePointsEqual(l1->endPoint(), l2->startPoint(), tolerance);
                        isDup = directMatch || reverseMatch;
                    }
                    break;
                }
                case EntityType::Circle: {
                    auto c1 = std::dynamic_pointer_cast<CircleEntity>(e1);
                    auto c2 = std::dynamic_pointer_cast<CircleEntity>(e2);
                    if (c1 && c2) {
                        isDup = arePointsEqual(c1->center(), c2->center(), tolerance) &&
                                std::abs(c1->radius() - c2->radius()) <= tolerance;
                    }
                    break;
                }
                case EntityType::Rectangle: {
                    auto r1 = std::dynamic_pointer_cast<RectangleEntity>(e1);
                    auto r2 = std::dynamic_pointer_cast<RectangleEntity>(e2);
                    if (r1 && r2) {
                        isDup = arePointsEqual(r1->topLeft(), r2->topLeft(), tolerance) &&
                                std::abs(r1->width() - r2->width()) <= tolerance &&
                                std::abs(r1->height() - r2->height()) <= tolerance;
                    }
                    break;
                }
                case EntityType::Text: {
                    auto t1 = std::dynamic_pointer_cast<TextEntity>(e1);
                    auto t2 = std::dynamic_pointer_cast<TextEntity>(e2);
                    if (t1 && t2) {
                        isDup = arePointsEqual(t1->position(), t2->position(), tolerance) &&
                                t1->text() == t2->text();
                    }
                    break;
                }
                default:
                    break;
            }

            if (isDup) {
                markedDuplicates.insert(e2->id());
                report.duplicates.push_back({
                    e1->id(),
                    e2->id(),
                    CadEntity::entityTypeToString(e1->type()),
                    QStringLiteral("Duplicate %1 detected").arg(CadEntity::entityTypeToString(e1->type()))
                });
            }
        }
    }

    // 3. Overlapping colinear line segments (excluding duplicates)
    for (size_t i = 0; i < lines.size(); ++i) {
        for (size_t j = i + 1; j < lines.size(); ++j) {
            if (markedDuplicates.count(lines[i]->id()) || markedDuplicates.count(lines[j]->id())) continue;
            if (areLinesColinearAndOverlapping(lines[i].get(), lines[j].get(), tolerance)) {
                report.overlaps.push_back({
                    lines[i]->id(),
                    lines[j]->id(),
                    QStringLiteral("Colinear lines overlap each other")
                });
            }
        }
    }

    // 4. Missing Dimensions
    if (geometryCount > 0 && dimensionCount == 0) {
        report.missingDimensionsCount = geometryCount;
        report.suggestions.append(QStringLiteral("Drawing has geometric shapes but no dimensions. Consider generating dimensions."));
    }

    // Suggestions
    if (!report.duplicates.empty()) {
        report.suggestions.append(QStringLiteral("Detected %1 duplicate entities. Run Auto-Clean to remove them.")
                                  .arg(report.duplicates.size()));
    }
    if (!report.overlaps.empty()) {
        report.suggestions.append(QStringLiteral("Detected %1 overlapping lines. Inspect or trim overlaps.")
                                  .arg(report.overlaps.size()));
    }
    if (!report.layerIssues.empty()) {
        report.suggestions.append(QStringLiteral("%1 entities reside on Layer '0'. Organize into functional layers.")
                                  .arg(report.layerIssues.size()));
    }

    return report;
}

AiProposal AiGeometryAnalyzer::generateCleanupProposal(const CadDocument* doc, double tolerance) {
    AiProposal proposal;
    proposal.description = QStringLiteral("Auto-Clean duplicate geometry");
    proposal.prompt = QStringLiteral("Clean duplicate entities");

    AiAnalysisReport report = analyze(doc, tolerance);
    for (const auto& dup : report.duplicates) {
        AiOperation op;
        op.type = AiOperationType::DeleteEntity;
        op.targetId = dup.duplicateId;
        proposal.operations.push_back(op);
    }

    if (proposal.operations.empty()) {
        proposal.description = QStringLiteral("No duplicates found to clean");
    }

    return proposal;
}

QJsonObject AiAnalysisReport::toJson() const {
    QJsonObject root;
    root["total_entities"] = totalEntities;
    root["total_wire_length"] = totalWireLength;
    root["has_issues"] = hasIssues();

    QJsonObject counts;
    for (const auto& pair : entityCountsByType) {
        counts[pair.first] = pair.second;
    }
    root["entity_counts"] = counts;

    QJsonObject bbox;
    bbox["x"] = boundingBox.x();
    bbox["y"] = boundingBox.y();
    bbox["width"] = boundingBox.width();
    bbox["height"] = boundingBox.height();
    root["bounding_box"] = bbox;

    QJsonArray dupsArr;
    for (const auto& d : duplicates) {
        QJsonObject dObj;
        dObj["primary_id"] = d.primaryId.toString(QUuid::WithoutBraces);
        dObj["duplicate_id"] = d.duplicateId.toString(QUuid::WithoutBraces);
        dObj["entity_type"] = d.entityType;
        dObj["description"] = d.description;
        dupsArr.append(dObj);
    }
    root["duplicates"] = dupsArr;

    QJsonArray overlapsArr;
    for (const auto& o : overlaps) {
        QJsonObject oObj;
        oObj["id1"] = o.entityId1.toString(QUuid::WithoutBraces);
        oObj["id2"] = o.entityId2.toString(QUuid::WithoutBraces);
        oObj["description"] = o.description;
        overlapsArr.append(oObj);
    }
    root["overlaps"] = overlapsArr;

    QJsonArray layerIssuesArr;
    for (const auto& l : layerIssues) {
        QJsonObject lObj;
        lObj["id"] = l.entityId.toString(QUuid::WithoutBraces);
        lObj["layer"] = l.currentLayer;
        lObj["description"] = l.issueDescription;
        layerIssuesArr.append(lObj);
    }
    root["layer_issues"] = layerIssuesArr;

    QJsonArray suggArr;
    for (const auto& s : suggestions) {
        suggArr.append(s);
    }
    root["suggestions"] = suggArr;

    return root;
}

QString AiAnalysisReport::summaryText() const {
    QStringList lines;
    lines << QStringLiteral("Drawing Statistics:");
    lines << QStringLiteral("  Total entities: %1").arg(totalEntities);
    lines << QStringLiteral("  Total wire length: %1 mm").arg(totalWireLength, 0, 'f', 2);
    lines << QStringLiteral("  Bounding box: [%1 x %2] at (%3, %4)")
             .arg(boundingBox.width(), 0, 'f', 1)
             .arg(boundingBox.height(), 0, 'f', 1)
             .arg(boundingBox.x(), 0, 'f', 1)
             .arg(boundingBox.y(), 0, 'f', 1);

    lines << QStringLiteral("\nAnalysis Findings:");
    if (!duplicates.empty()) {
        lines << QStringLiteral("  - Duplicate entities found: %1").arg(duplicates.size());
    } else {
        lines << QStringLiteral("  - No duplicate entities found (Clean)");
    }

    if (!overlaps.empty()) {
        lines << QStringLiteral("  - Overlapping colinear lines: %1").arg(overlaps.size());
    } else {
        lines << QStringLiteral("  - No overlapping lines detected");
    }

    if (!layerIssues.empty()) {
        lines << QStringLiteral("  - Entities on default Layer 0: %1").arg(layerIssues.size());
    }

    if (missingDimensionsCount > 0) {
        lines << QStringLiteral("  - Missing dimensions: %1 geometry elements unannotated").arg(missingDimensionsCount);
    }

    return lines.join(QStringLiteral("\n"));
}
