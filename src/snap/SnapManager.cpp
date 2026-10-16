#include "snap/SnapManager.h"
#include "document/CadDocument.h"
#include "document/CoordinateSystem.h"
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
#include <QPolygonF>
#include <cmath>
#include <algorithm>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SnapManager::SnapManager(QObject* parent)
    : QObject(parent)
{
}

void SnapManager::setSnapEnabled(bool enabled) {
    if (m_snapEnabled != enabled) {
        m_snapEnabled = enabled;
        emit snapSettingsChanged();
    }
}

void SnapManager::setGridSnapEnabled(bool enabled) {
    if (m_gridSnapEnabled != enabled) {
        m_gridSnapEnabled = enabled;
        emit snapSettingsChanged();
    }
}

void SnapManager::setPolarEnabled(bool enabled) {
    if (m_polarEnabled != enabled) {
        m_polarEnabled = enabled;
        emit polarToggled(m_polarEnabled);
        emit snapSettingsChanged();
    }
}

void SnapManager::setPolarAngleIncrement(double deg) {
    if (deg > 0.1 && deg <= 180.0) {
        m_polarAngleIncrement = deg;
        emit snapSettingsChanged();
    }
}

void SnapManager::setBasePoint(const QPointF& pt, bool active) {
    m_basePoint = pt;
    m_hasBasePoint = active;
}

void SnapManager::clearBasePoint() {
    m_hasBasePoint = false;
}

void SnapManager::setModeEnabled(SnapType type, bool enabled) {
    int mask = static_cast<int>(type);
    if (enabled) {
        m_enabledModes |= mask;
    } else {
        m_enabledModes &= ~mask;
    }
    emit snapSettingsChanged();
}

bool SnapManager::isModeEnabled(SnapType type) const {
    return (m_enabledModes & static_cast<int>(type)) != 0;
}

namespace {
// Helper: Line segment intersection
bool lineSegmentsIntersect(const QPointF& p1, const QPointF& p2,
                           const QPointF& p3, const QPointF& p4,
                           QPointF* outPt) {
    double d = (p2.x() - p1.x()) * (p4.y() - p3.y()) - (p2.y() - p1.y()) * (p4.x() - p3.x());
    if (std::abs(d) < 1e-9) return false;

    double u = ((p3.x() - p1.x()) * (p4.y() - p3.y()) - (p3.y() - p1.y()) * (p4.x() - p3.x())) / d;
    double v = ((p3.x() - p1.x()) * (p2.y() - p1.y()) - (p3.y() - p1.y()) * (p2.x() - p1.x())) / d;

    if (u >= -1e-4 && u <= 1.0 + 1e-4 && v >= -1e-4 && v <= 1.0 + 1e-4) {
        if (outPt) {
            outPt->setX(p1.x() + u * (p2.x() - p1.x()));
            outPt->setY(p1.y() + u * (p2.y() - p1.y()));
        }
        return true;
    }
    return false;
}

// Helper: Project point onto segment
QPointF projectPointOnSegment(const QPointF& pt, const QPointF& a, const QPointF& b, double* outT = nullptr) {
    double dx = b.x() - a.x();
    double dy = b.y() - a.y();
    double lenSq = dx * dx + dy * dy;
    if (lenSq < 1e-12) {
        if (outT) *outT = 0.0;
        return a;
    }
    double t = std::clamp(((pt.x() - a.x()) * dx + (pt.y() - a.y()) * dy) / lenSq, 0.0, 1.0);
    if (outT) *outT = t;
    return QPointF(a.x() + t * dx, a.y() + t * dy);
}
}

SnapResult SnapManager::findSnap(const QPointF& scenePos, const CadDocument* doc, double snapRadius) const {
    SnapResult bestResult;
    bestResult.snapped = false;
    double minDistance = snapRadius;

    struct Candidate {
        QPointF point;
        SnapType type;
        QString desc;
    };

    if (m_snapEnabled && doc) {
        std::vector<Candidate> candidates;
        std::vector<QPair<QPointF, QPointF>> allSegments;

        for (const auto& entity : doc->entities()) {
            if (!entity) continue;

            if (doc->layerManager()) {
                auto layer = doc->layerManager()->findLayer(entity->layer());
                if (layer && !layer->isVisible()) continue;
            }

            switch (entity->type()) {
                case EntityType::Line: {
                    const auto* line = static_cast<const LineEntity*>(entity.get());
                    allSegments.push_back({line->p1(), line->p2()});
                    if (isModeEnabled(SnapType::Endpoint)) {
                        candidates.push_back({line->p1(), SnapType::Endpoint, QStringLiteral("Endpoint")});
                        candidates.push_back({line->p2(), SnapType::Endpoint, QStringLiteral("Endpoint")});
                    }
                    if (isModeEnabled(SnapType::Midpoint)) {
                        candidates.push_back({line->midPoint(), SnapType::Midpoint, QStringLiteral("Midpoint")});
                    }
                    if (isModeEnabled(SnapType::Perpendicular) && m_hasBasePoint) {
                        double t = 0.0;
                        QPointF perpPt = projectPointOnSegment(m_basePoint, line->p1(), line->p2(), &t);
                        candidates.push_back({perpPt, SnapType::Perpendicular, QStringLiteral("Perpendicular")});
                    }
                    if (isModeEnabled(SnapType::Nearest)) {
                        QPointF nearPt = projectPointOnSegment(scenePos, line->p1(), line->p2());
                        candidates.push_back({nearPt, SnapType::Nearest, QStringLiteral("Nearest")});
                    }
                    break;
                }
                case EntityType::Circle: {
                    const auto* circle = static_cast<const CircleEntity*>(entity.get());
                    if (isModeEnabled(SnapType::Center)) {
                        candidates.push_back({circle->center(), SnapType::Center, QStringLiteral("Center")});
                    }
                    if (isModeEnabled(SnapType::Quadrant)) {
                        QPointF c = circle->center();
                        double r = circle->radius();
                        candidates.push_back({QPointF(c.x() + r, c.y()), SnapType::Quadrant, QStringLiteral("Quadrant")});
                        candidates.push_back({QPointF(c.x() - r, c.y()), SnapType::Quadrant, QStringLiteral("Quadrant")});
                        candidates.push_back({QPointF(c.x(), c.y() + r), SnapType::Quadrant, QStringLiteral("Quadrant")});
                        candidates.push_back({QPointF(c.x(), c.y() - r), SnapType::Quadrant, QStringLiteral("Quadrant")});
                    }
                    if (isModeEnabled(SnapType::Nearest)) {
                        QPointF c = circle->center();
                        double dx = scenePos.x() - c.x();
                        double dy = scenePos.y() - c.y();
                        double dist = std::hypot(dx, dy);
                        if (dist > 1e-4) {
                            candidates.push_back({QPointF(c.x() + dx / dist * circle->radius(),
                                                          c.y() + dy / dist * circle->radius()),
                                                  SnapType::Nearest, QStringLiteral("Nearest")});
                        }
                    }
                    if (isModeEnabled(SnapType::Tangent) && m_hasBasePoint) {
                        QPointF c = circle->center();
                        double d = std::hypot(m_basePoint.x() - c.x(), m_basePoint.y() - c.y());
                        double r = circle->radius();
                        if (d >= r && d > 1e-4) {
                            double alpha = std::asin(std::clamp(r / d, -1.0, 1.0));
                            double baseAngle = std::atan2(c.y() - m_basePoint.y(), c.x() - m_basePoint.x());
                            double tanDist = std::sqrt(std::max(0.0, d * d - r * r));
                            candidates.push_back({QPointF(m_basePoint.x() + tanDist * std::cos(baseAngle + alpha),
                                                          m_basePoint.y() + tanDist * std::sin(baseAngle + alpha)),
                                                  SnapType::Tangent, QStringLiteral("Tangent")});
                            candidates.push_back({QPointF(m_basePoint.x() + tanDist * std::cos(baseAngle - alpha),
                                                          m_basePoint.y() + tanDist * std::sin(baseAngle - alpha)),
                                                  SnapType::Tangent, QStringLiteral("Tangent")});
                        }
                    }
                    break;
                }
                case EntityType::Rectangle: {
                    const auto* rect = static_cast<const RectangleEntity*>(entity.get());
                    const auto& c = rect->corners();
                    for (size_t i = 0; i < 4; ++i) {
                        allSegments.push_back({c[i], c[(i + 1) % 4]});
                        if (isModeEnabled(SnapType::Endpoint)) {
                            candidates.push_back({c[i], SnapType::Endpoint, QStringLiteral("Corner")});
                        }
                        if (isModeEnabled(SnapType::Midpoint)) {
                            candidates.push_back({(c[i] + c[(i + 1) % 4]) * 0.5, SnapType::Midpoint, QStringLiteral("Midpoint")});
                        }
                        if (isModeEnabled(SnapType::Perpendicular) && m_hasBasePoint) {
                            candidates.push_back({projectPointOnSegment(m_basePoint, c[i], c[(i + 1) % 4]), SnapType::Perpendicular, QStringLiteral("Perpendicular")});
                        }
                        if (isModeEnabled(SnapType::Nearest)) {
                            candidates.push_back({projectPointOnSegment(scenePos, c[i], c[(i + 1) % 4]), SnapType::Nearest, QStringLiteral("Nearest")});
                        }
                    }
                    if (isModeEnabled(SnapType::Center)) {
                        candidates.push_back({(c[0] + c[2]) * 0.5, SnapType::Center, QStringLiteral("Center")});
                    }
                    break;
                }
                case EntityType::Arc: {
                    const auto* arc = static_cast<const ArcEntity*>(entity.get());
                    if (isModeEnabled(SnapType::Center)) {
                        candidates.push_back({arc->center(), SnapType::Center, QStringLiteral("Center")});
                    }
                    if (isModeEnabled(SnapType::Endpoint)) {
                        candidates.push_back({arc->startPoint(), SnapType::Endpoint, QStringLiteral("Endpoint")});
                        candidates.push_back({arc->endPoint(), SnapType::Endpoint, QStringLiteral("Endpoint")});
                    }
                    if (isModeEnabled(SnapType::Midpoint)) {
                        candidates.push_back({arc->midPoint(), SnapType::Midpoint, QStringLiteral("Midpoint")});
                    }
                    if (isModeEnabled(SnapType::Nearest)) {
                        QPointF c = arc->center();
                        double dx = scenePos.x() - c.x();
                        double dy = scenePos.y() - c.y();
                        double dist = std::hypot(dx, dy);
                        if (dist > 1e-4) {
                            candidates.push_back({QPointF(c.x() + dx / dist * arc->radius(),
                                                          c.y() + dy / dist * arc->radius()),
                                                  SnapType::Nearest, QStringLiteral("Nearest")});
                        }
                    }
                    break;
                }
                case EntityType::Polyline: {
                    const auto* poly = static_cast<const PolylineEntity*>(entity.get());
                    const auto& pts = poly->points();
                    if (pts.size() >= 2) {
                        size_t segCount = poly->isClosed() ? pts.size() : pts.size() - 1;
                        for (size_t i = 0; i < segCount; ++i) {
                            const QPointF& p1 = pts[i];
                            const QPointF& p2 = pts[(i + 1) % pts.size()];
                            allSegments.push_back({p1, p2});
                            if (isModeEnabled(SnapType::Endpoint)) {
                                candidates.push_back({p1, SnapType::Endpoint, QStringLiteral("Vertex")});
                            }
                            if (isModeEnabled(SnapType::Midpoint)) {
                                candidates.push_back({(p1 + p2) * 0.5, SnapType::Midpoint, QStringLiteral("Midpoint")});
                            }
                            if (isModeEnabled(SnapType::Nearest)) {
                                candidates.push_back({projectPointOnSegment(scenePos, p1, p2), SnapType::Nearest, QStringLiteral("Nearest")});
                            }
                        }
                        if (!poly->isClosed() && isModeEnabled(SnapType::Endpoint)) {
                            candidates.push_back({pts.back(), SnapType::Endpoint, QStringLiteral("Vertex")});
                        }
                    }
                    break;
                }
                case EntityType::Polygon: {
                    const auto* polygon = static_cast<const PolygonEntity*>(entity.get());
                    if (isModeEnabled(SnapType::Center)) {
                        candidates.push_back({polygon->center(), SnapType::Center, QStringLiteral("Center")});
                    }
                    auto v = polygon->vertices();
                    for (size_t i = 0; i < v.size(); ++i) {
                        const QPointF& p1 = v[i];
                        const QPointF& p2 = v[(i + 1) % v.size()];
                        allSegments.push_back({p1, p2});
                        if (isModeEnabled(SnapType::Endpoint)) {
                            candidates.push_back({p1, SnapType::Endpoint, QStringLiteral("Vertex")});
                        }
                        if (isModeEnabled(SnapType::Midpoint)) {
                            candidates.push_back({(p1 + p2) * 0.5, SnapType::Midpoint, QStringLiteral("Midpoint")});
                        }
                        if (isModeEnabled(SnapType::Nearest)) {
                            candidates.push_back({projectPointOnSegment(scenePos, p1, p2), SnapType::Nearest, QStringLiteral("Nearest")});
                        }
                    }
                    break;
                }
                case EntityType::Ellipse: {
                    const auto* ellipse = static_cast<const EllipseEntity*>(entity.get());
                    auto snaps = ellipse->snapPoints();
                    if (!snaps.empty() && isModeEnabled(SnapType::Center)) {
                        candidates.push_back({snaps[0], SnapType::Center, QStringLiteral("Center")});
                    }
                    if (isModeEnabled(SnapType::Quadrant)) {
                        for (size_t i = 1; i < snaps.size(); ++i) {
                            candidates.push_back({snaps[i], SnapType::Quadrant, QStringLiteral("Quadrant")});
                        }
                    }
                    break;
                }
                case EntityType::Text: {
                    const auto* text = static_cast<const TextEntity*>(entity.get());
                    if (isModeEnabled(SnapType::Endpoint)) {
                        candidates.push_back({text->position(), SnapType::Endpoint, QStringLiteral("Insert Point")});
                    }
                    break;
                }
                case EntityType::Dimension: {
                    const auto* dim = static_cast<const DimensionEntity*>(entity.get());
                    if (isModeEnabled(SnapType::Endpoint)) {
                        candidates.push_back({dim->p1(), SnapType::Endpoint, QStringLiteral("Endpoint")});
                        candidates.push_back({dim->p2(), SnapType::Endpoint, QStringLiteral("Endpoint")});
                    }
                    if (isModeEnabled(SnapType::Midpoint)) {
                        candidates.push_back({(dim->p1() + dim->p2()) * 0.5, SnapType::Midpoint, QStringLiteral("Midpoint")});
                    }
                    break;
                }
            }
        }

        // Calculate intersections between segments
        if (isModeEnabled(SnapType::Intersection) && allSegments.size() > 1) {
            for (size_t i = 0; i < allSegments.size(); ++i) {
                for (size_t j = i + 1; j < allSegments.size(); ++j) {
                    QPointF interPt;
                    if (lineSegmentsIntersect(allSegments[i].first, allSegments[i].second,
                                              allSegments[j].first, allSegments[j].second, &interPt)) {
                        candidates.push_back({interPt, SnapType::Intersection, QStringLiteral("Intersection")});
                    }
                }
            }
        }

        // Select closest candidate: discrete snaps take precedence over Nearest
        for (const auto& cand : candidates) {
            if (cand.type == SnapType::Nearest) continue;
            double dist = std::hypot(cand.point.x() - scenePos.x(), cand.point.y() - scenePos.y());
            if (dist < minDistance) {
                minDistance = dist;
                bestResult.snapped = true;
                bestResult.point = cand.point;
                bestResult.type = cand.type;
                bestResult.description = cand.desc;
            }
        }

        // If no discrete snap matched, consider Nearest candidates
        if (!bestResult.snapped) {
            for (const auto& cand : candidates) {
                if (cand.type != SnapType::Nearest) continue;
                double dist = std::hypot(cand.point.x() - scenePos.x(), cand.point.y() - scenePos.y());
                if (dist < minDistance) {
                    minDistance = dist;
                    bestResult.snapped = true;
                    bestResult.point = cand.point;
                    bestResult.type = cand.type;
                    bestResult.description = cand.desc;
                }
            }
        }
    }

    // Polar tracking evaluation if no high-priority entity snap found
    if (!bestResult.snapped && m_polarEnabled && m_hasBasePoint) {
        bool polarSnapped = false;
        QPointF polarPt = CoordinateSystem::snapToPolar(m_basePoint, scenePos, m_polarAngleIncrement, 3.5, &polarSnapped);
        if (polarSnapped) {
            bestResult.snapped = true;
            bestResult.point = polarPt;
            bestResult.type = SnapType::Polar;
            bestResult.hasPolarLine = true;
            bestResult.polarStart = m_basePoint;
            bestResult.polarEnd = polarPt;
            bestResult.polarAngleDeg = CoordinateSystem::angleDeg(m_basePoint, polarPt);
            bestResult.description = QStringLiteral("Polar %1°").arg(bestResult.polarAngleDeg, 0, 'f', 1);
        }
    }

    // Grid snap fallback if still not snapped
    if (!bestResult.snapped && m_gridSnapEnabled && m_gridSize > 0.01) {
        double gx = std::round(scenePos.x() / m_gridSize) * m_gridSize;
        double gy = std::round(scenePos.y() / m_gridSize) * m_gridSize;
        QPointF gridPt(gx, gy);
        double dist = std::hypot(gridPt.x() - scenePos.x(), gridPt.y() - scenePos.y());
        if (dist <= snapRadius) {
            bestResult.snapped = true;
            bestResult.point = gridPt;
            bestResult.type = SnapType::Grid;
            bestResult.description = QStringLiteral("Grid (%1, %2)").arg(gx).arg(gy);
        }
    }

    return bestResult;
}

void SnapManager::drawSnapMarker(QPainter* painter, const SnapResult& snap, double zoomFactor) {
    if (!snap.snapped) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    double s = 6.0 / (zoomFactor > 0.001 ? zoomFactor : 1.0);
    const QPointF& pt = snap.point;

    // Draw Polar tracking dashed ray
    if (snap.hasPolarLine) {
        QPen polarPen(QColor(0, 230, 255, 180), 1.0, Qt::DashLine);
        polarPen.setCosmetic(true);
        painter->setPen(polarPen);

        // Extend the polar line across the view
        double dx = snap.polarEnd.x() - snap.polarStart.x();
        double dy = snap.polarEnd.y() - snap.polarStart.y();
        double len = std::hypot(dx, dy);
        if (len > 1e-4) {
            QPointF ext = snap.polarStart + QPointF(dx / len * (len + 200.0 / zoomFactor),
                                                   dy / len * (len + 200.0 / zoomFactor));
            painter->drawLine(snap.polarStart, ext);
        }
    }

    QColor snapColor(0, 255, 128); // Vibrant CAD snap green
    if (snap.type == SnapType::Polar) {
        snapColor = QColor(0, 230, 255);
    } else if (snap.type == SnapType::Intersection) {
        snapColor = QColor(255, 180, 0);
    } else if (snap.type == SnapType::Perpendicular || snap.type == SnapType::Tangent) {
        snapColor = QColor(255, 100, 255);
    }

    QPen pen(snapColor, 1.8);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    switch (snap.type) {
        case SnapType::Endpoint:
            // Square marker
            painter->drawRect(QRectF(pt.x() - s, pt.y() - s, 2 * s, 2 * s));
            break;
        case SnapType::Midpoint: {
            // Triangle marker
            QPolygonF tri;
            tri << QPointF(pt.x(), pt.y() - s * 1.2)
                << QPointF(pt.x() - s, pt.y() + s * 0.8)
                << QPointF(pt.x() + s, pt.y() + s * 0.8);
            painter->drawPolygon(tri);
            break;
        }
        case SnapType::Center:
            // Circle marker with center dot
            painter->drawEllipse(pt, s, s);
            painter->drawPoint(pt);
            break;
        case SnapType::Quadrant: {
            // Diamond marker
            QPolygonF diamond;
            diamond << QPointF(pt.x(), pt.y() - s)
                    << QPointF(pt.x() + s, pt.y())
                    << QPointF(pt.x(), pt.y() + s)
                    << QPointF(pt.x() - s, pt.y());
            painter->drawPolygon(diamond);
            break;
        }
        case SnapType::Intersection:
            // X mark
            painter->drawLine(QPointF(pt.x() - s, pt.y() - s), QPointF(pt.x() + s, pt.y() + s));
            painter->drawLine(QPointF(pt.x() - s, pt.y() + s), QPointF(pt.x() + s, pt.y() - s));
            break;
        case SnapType::Perpendicular:
            // Right-angle symbol
            painter->drawLine(QPointF(pt.x() - s, pt.y() - s), QPointF(pt.x() + s, pt.y() - s));
            painter->drawLine(QPointF(pt.x() - s, pt.y() - s), QPointF(pt.x() - s, pt.y() + s));
            painter->drawRect(QRectF(pt.x() - s, pt.y() - s, s, s));
            break;
        case SnapType::Tangent:
            // Circle with tangent line
            painter->drawEllipse(pt, s, s);
            painter->drawLine(QPointF(pt.x() - s * 1.2, pt.y() - s), QPointF(pt.x() + s * 1.2, pt.y() - s));
            break;
        case SnapType::Nearest: {
            // Hourglass / bowtie marker
            QPolygonF bowtie;
            bowtie << QPointF(pt.x() - s, pt.y() - s)
                   << QPointF(pt.x() + s, pt.y() + s)
                   << QPointF(pt.x() - s, pt.y() + s)
                   << QPointF(pt.x() + s, pt.y() - s);
            painter->drawPolygon(bowtie);
            break;
        }
        case SnapType::Grid:
            // Crosshair
            painter->drawLine(QPointF(pt.x() - s, pt.y()), QPointF(pt.x() + s, pt.y()));
            painter->drawLine(QPointF(pt.x(), pt.y() - s), QPointF(pt.x(), pt.y() + s));
            break;
        case SnapType::Polar:
            // Small cross with ray
            painter->drawLine(QPointF(pt.x() - s * 0.7, pt.y()), QPointF(pt.x() + s * 0.7, pt.y()));
            painter->drawLine(QPointF(pt.x(), pt.y() - s * 0.7), QPointF(pt.x(), pt.y() + s * 0.7));
            break;
        default:
            painter->drawRect(QRectF(pt.x() - s, pt.y() - s, 2 * s, 2 * s));
            break;
    }

    // Draw snap tooltip badge
    if (!snap.description.isEmpty()) {
        QFont font = painter->font();
        font.setPointSizeF(8.5);
        painter->setFont(font);
        painter->setPen(QPen(snapColor, 1));
        painter->setBrush(QColor(20, 20, 25, 210));
        QRectF textRect(pt.x() + s + 3.0 / zoomFactor, pt.y() - s - 14.0 / zoomFactor,
                        (snap.description.length() * 7.0 + 8.0) / zoomFactor, 16.0 / zoomFactor);
        painter->drawRoundedRect(textRect, 2.0 / zoomFactor, 2.0 / zoomFactor);
        painter->drawText(textRect, Qt::AlignCenter, snap.description);
    }

    painter->restore();
}
