#include "geometry/EllipseEntity.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

EllipseEntity::EllipseEntity()
    : CadEntity(EntityType::Ellipse)
    , m_center(0.0, 0.0)
    , m_radiusX(100.0)
    , m_radiusY(50.0)
    , m_rotationDeg(0.0)
{
}

EllipseEntity::EllipseEntity(const QPointF& center, double radiusX, double radiusY, double rotationDeg)
    : CadEntity(EntityType::Ellipse)
    , m_center(center)
    , m_radiusX(std::max(1e-4, radiusX))
    , m_radiusY(std::max(1e-4, radiusY))
    , m_rotationDeg(rotationDeg)
{
}

EllipseEntity::EllipseEntity(const QUuid& id, const QPointF& center, double radiusX, double radiusY, double rotationDeg)
    : CadEntity(EntityType::Ellipse, id)
    , m_center(center)
    , m_radiusX(std::max(1e-4, radiusX))
    , m_radiusY(std::max(1e-4, radiusY))
    , m_rotationDeg(rotationDeg)
{
}

void EllipseEntity::setRadiusX(double rx) {
    if (rx > 1e-4) m_radiusX = rx;
}

void EllipseEntity::setRadiusY(double ry) {
    if (ry > 1e-4) m_radiusY = ry;
}

double EllipseEntity::area() const {
    return M_PI * m_radiusX * m_radiusY;
}

double EllipseEntity::perimeter() const {
    // Ramanujan approximation for ellipse perimeter
    double a = std::max(m_radiusX, m_radiusY);
    double b = std::min(m_radiusX, m_radiusY);
    double h = ((a - b) * (a - b)) / ((a + b) * (a + b));
    return M_PI * (a + b) * (1.0 + (3.0 * h) / (10.0 + std::sqrt(4.0 - 3.0 * h)));
}

std::shared_ptr<CadEntity> EllipseEntity::clone() const {
    auto copy = std::make_shared<EllipseEntity>(m_id, m_center, m_radiusX, m_radiusY, m_rotationDeg);
    copy->setColor(m_color);
    copy->setStrokeWidth(m_strokeWidth);
    copy->setSelected(m_selected);
    copy->setLayer(m_layer);
    return copy;
}

QRectF EllipseEntity::boundingRect() const {
    double maxR = std::max(m_radiusX, m_radiusY);
    return QRectF(m_center.x() - maxR, m_center.y() - maxR, 2.0 * maxR, 2.0 * maxR).adjusted(-2, -2, 2, 2);
}

void EllipseEntity::move(double deltaX, double deltaY) {
    m_center.rx() += deltaX;
    m_center.ry() += deltaY;
}

void EllipseEntity::rotate(const QPointF& origin, double angleRad) {
    m_center = rotatePoint(m_center, origin, angleRad);
    m_rotationDeg += angleRad * 180.0 / M_PI;
    while (m_rotationDeg >= 360.0) m_rotationDeg -= 360.0;
    while (m_rotationDeg < 0.0) m_rotationDeg += 360.0;
}

void EllipseEntity::mirror(const QPointF& p1, const QPointF& p2) {
    m_center = mirrorPoint(m_center, p1, p2);
    double lineAngle = std::atan2(p2.y() - p1.y(), p2.x() - p1.x()) * 180.0 / M_PI;
    m_rotationDeg = 2.0 * lineAngle - m_rotationDeg;
    while (m_rotationDeg >= 360.0) m_rotationDeg -= 360.0;
    while (m_rotationDeg < 0.0) m_rotationDeg += 360.0;
}

bool EllipseEntity::hitTest(const QPointF& worldPt, double tolerance) const {
    // Transform point to local unrotated coordinate space
    double rad = -m_rotationDeg * M_PI / 180.0;
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);
    double dx = worldPt.x() - m_center.x();
    double dy = worldPt.y() - m_center.y();
    double localX = dx * cosA - dy * sinA;
    double localY = dx * sinA + dy * cosA;

    // Check boundary proximity
    double normDist = std::hypot(localX / m_radiusX, localY / m_radiusY);
    // Approximate distance to ellipse edge
    double avgRadius = (m_radiusX + m_radiusY) * 0.5;
    double distToEdge = std::abs(normDist - 1.0) * avgRadius;
    if (distToEdge <= tolerance) return true;

    // Or inside the ellipse
    return normDist <= 1.0;
}

std::vector<QPointF> EllipseEntity::snapPoints() const {
    std::vector<QPointF> snaps;
    snaps.push_back(m_center); // Center

    double rad = m_rotationDeg * M_PI / 180.0;
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);

    // 4 Quadrants
    snaps.emplace_back(m_center.x() + m_radiusX * cosA, m_center.y() + m_radiusX * sinA);
    snaps.emplace_back(m_center.x() - m_radiusX * cosA, m_center.y() - m_radiusX * sinA);
    snaps.emplace_back(m_center.x() - m_radiusY * sinA, m_center.y() + m_radiusY * cosA);
    snaps.emplace_back(m_center.x() + m_radiusY * sinA, m_center.y() - m_radiusY * cosA);

    return snaps;
}

QJsonObject EllipseEntity::toJson() const {
    QJsonObject obj = CadEntity::toJson();
    obj[QStringLiteral("centerX")] = m_center.x();
    obj[QStringLiteral("centerY")] = m_center.y();
    obj[QStringLiteral("radiusX")] = m_radiusX;
    obj[QStringLiteral("radiusY")] = m_radiusY;
    obj[QStringLiteral("rotationDeg")] = m_rotationDeg;
    return obj;
}

bool EllipseEntity::fromJson(const QJsonObject& json) {
    if (!CadEntity::fromJson(json)) return false;
    m_center.setX(json[QStringLiteral("centerX")].toDouble(0.0));
    m_center.setY(json[QStringLiteral("centerY")].toDouble(0.0));
    m_radiusX = std::max(1e-4, json[QStringLiteral("radiusX")].toDouble(100.0));
    m_radiusY = std::max(1e-4, json[QStringLiteral("radiusY")].toDouble(50.0));
    m_rotationDeg = json[QStringLiteral("rotationDeg")].toDouble(0.0);
    return true;
}
