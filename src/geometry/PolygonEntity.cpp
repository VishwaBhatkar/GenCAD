#include "geometry/PolygonEntity.h"
#include <cmath>
#include <algorithm>
#include <QPolygonF>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PolygonEntity::PolygonEntity()
    : CadEntity(EntityType::Polygon)
    , m_center(0.0, 0.0)
    , m_sides(6)
    , m_radius(100.0)
    , m_rotationDeg(0.0)
{
}

PolygonEntity::PolygonEntity(const QPointF& center, int sides, double radius, double rotationDeg)
    : CadEntity(EntityType::Polygon)
    , m_center(center)
    , m_sides(std::max(3, sides))
    , m_radius(std::max(1e-4, radius))
    , m_rotationDeg(rotationDeg)
{
}

PolygonEntity::PolygonEntity(const QUuid& id, const QPointF& center, int sides, double radius, double rotationDeg)
    : CadEntity(EntityType::Polygon, id)
    , m_center(center)
    , m_sides(std::max(3, sides))
    , m_radius(std::max(1e-4, radius))
    , m_rotationDeg(rotationDeg)
{
}

void PolygonEntity::setSides(int sides) {
    m_sides = std::max(3, sides);
}

void PolygonEntity::setRadius(double r) {
    if (r > 1e-4) m_radius = r;
}

std::vector<QPointF> PolygonEntity::vertices() const {
    std::vector<QPointF> pts;
    pts.reserve(m_sides);
    double startRad = m_rotationDeg * M_PI / 180.0;
    double step = 2.0 * M_PI / m_sides;
    for (int i = 0; i < m_sides; ++i) {
        double angle = startRad + i * step;
        pts.emplace_back(m_center.x() + m_radius * std::cos(angle),
                         m_center.y() + m_radius * std::sin(angle));
    }
    return pts;
}

double PolygonEntity::sideLength() const {
    return 2.0 * m_radius * std::sin(M_PI / m_sides);
}

double PolygonEntity::area() const {
    return 0.5 * m_sides * m_radius * m_radius * std::sin(2.0 * M_PI / m_sides);
}

double PolygonEntity::perimeter() const {
    return m_sides * sideLength();
}

std::shared_ptr<CadEntity> PolygonEntity::clone() const {
    auto copy = std::make_shared<PolygonEntity>(m_id, m_center, m_sides, m_radius, m_rotationDeg);
    copy->setColor(m_color);
    copy->setStrokeWidth(m_strokeWidth);
    copy->setSelected(m_selected);
    copy->setLayer(m_layer);
    return copy;
}

QRectF PolygonEntity::boundingRect() const {
    auto pts = vertices();
    if (pts.empty()) return QRectF(m_center, QSizeF(0, 0));
    double minX = pts[0].x(), maxX = pts[0].x();
    double minY = pts[0].y(), maxY = pts[0].y();
    for (const auto& p : pts) {
        minX = std::min(minX, p.x());
        maxX = std::max(maxX, p.x());
        minY = std::min(minY, p.y());
        maxY = std::max(maxY, p.y());
    }
    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).adjusted(-2, -2, 2, 2);
}

void PolygonEntity::move(double deltaX, double deltaY) {
    m_center.rx() += deltaX;
    m_center.ry() += deltaY;
}

void PolygonEntity::rotate(const QPointF& origin, double angleRad) {
    m_center = rotatePoint(m_center, origin, angleRad);
    m_rotationDeg += angleRad * 180.0 / M_PI;
    while (m_rotationDeg >= 360.0) m_rotationDeg -= 360.0;
    while (m_rotationDeg < 0.0) m_rotationDeg += 360.0;
}

void PolygonEntity::mirror(const QPointF& p1, const QPointF& p2) {
    m_center = mirrorPoint(m_center, p1, p2);
    // Mirroring reverses winding direction
    double lineAngle = std::atan2(p2.y() - p1.y(), p2.x() - p1.x()) * 180.0 / M_PI;
    m_rotationDeg = 2.0 * lineAngle - m_rotationDeg;
    while (m_rotationDeg >= 360.0) m_rotationDeg -= 360.0;
    while (m_rotationDeg < 0.0) m_rotationDeg += 360.0;
}

bool PolygonEntity::hitTest(const QPointF& worldPt, double tolerance) const {
    auto pts = vertices();
    if (pts.size() < 3) return false;

    // Check distance to edges
    for (size_t i = 0; i < pts.size(); ++i) {
        const QPointF& a = pts[i];
        const QPointF& b = pts[(i + 1) % pts.size()];
        double dx = b.x() - a.x();
        double dy = b.y() - a.y();
        double lenSq = dx * dx + dy * dy;
        if (lenSq < 1e-12) continue;

        double t = std::clamp(((worldPt.x() - a.x()) * dx + (worldPt.y() - a.y()) * dy) / lenSq, 0.0, 1.0);
        double px = a.x() + t * dx;
        double py = a.y() + t * dy;
        if (std::hypot(worldPt.x() - px, worldPt.y() - py) <= tolerance) {
            return true;
        }
    }

    // Inside check
    QPolygonF poly;
    for (const auto& p : pts) poly << p;
    return poly.containsPoint(worldPt, Qt::OddEvenFill);
}

std::vector<QPointF> PolygonEntity::snapPoints() const {
    std::vector<QPointF> snaps;
    snaps.push_back(m_center); // Center
    auto pts = vertices();
    for (const auto& p : pts) {
        snaps.push_back(p); // Vertices
    }
    for (size_t i = 0; i < pts.size(); ++i) {
        snaps.push_back((pts[i] + pts[(i + 1) % pts.size()]) * 0.5); // Midpoints
    }
    return snaps;
}

QJsonObject PolygonEntity::toJson() const {
    QJsonObject obj = CadEntity::toJson();
    obj[QStringLiteral("centerX")] = m_center.x();
    obj[QStringLiteral("centerY")] = m_center.y();
    obj[QStringLiteral("sides")] = m_sides;
    obj[QStringLiteral("radius")] = m_radius;
    obj[QStringLiteral("rotationDeg")] = m_rotationDeg;
    return obj;
}

bool PolygonEntity::fromJson(const QJsonObject& json) {
    if (!CadEntity::fromJson(json)) return false;
    m_center.setX(json[QStringLiteral("centerX")].toDouble(0.0));
    m_center.setY(json[QStringLiteral("centerY")].toDouble(0.0));
    m_sides = std::max(3, json[QStringLiteral("sides")].toInt(6));
    m_radius = std::max(1e-4, json[QStringLiteral("radius")].toDouble(100.0));
    m_rotationDeg = json[QStringLiteral("rotationDeg")].toDouble(0.0);
    return true;
}
