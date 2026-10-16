#include "geometry/CircleEntity.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CircleEntity::CircleEntity()
    : CadEntity(EntityType::Circle)
    , m_center(0.0, 0.0)
    , m_radius(0.0)
{
}

CircleEntity::CircleEntity(const QPointF& center, double radius)
    : CadEntity(EntityType::Circle)
    , m_center(center)
    , m_radius(std::abs(radius))
{
}

CircleEntity::CircleEntity(const QUuid& id, const QPointF& center, double radius)
    : CadEntity(EntityType::Circle, id)
    , m_center(center)
    , m_radius(std::abs(radius))
{
}

double CircleEntity::area() const {
    return M_PI * m_radius * m_radius;
}

double CircleEntity::circumference() const {
    return 2.0 * M_PI * m_radius;
}

std::shared_ptr<CadEntity> CircleEntity::clone() const {
    auto copy = std::make_shared<CircleEntity>(m_id, m_center, m_radius);
    copy->setColor(m_color);
    copy->setStrokeWidth(m_strokeWidth);
    copy->setSelected(m_selected);
    copy->setLayer(m_layer);
    return copy;
}

QRectF CircleEntity::boundingRect() const {
    return QRectF(m_center.x() - m_radius,
                  m_center.y() - m_radius,
                  2.0 * m_radius,
                  2.0 * m_radius);
}

void CircleEntity::move(double deltaX, double deltaY) {
    m_center.rx() += deltaX;
    m_center.ry() += deltaY;
}

void CircleEntity::rotate(const QPointF& origin, double angleRad) {
    m_center = rotatePoint(m_center, origin, angleRad);
}

void CircleEntity::mirror(const QPointF& p1, const QPointF& p2) {
    m_center = mirrorPoint(m_center, p1, p2);
}

std::vector<QPointF> CircleEntity::snapPoints() const {
    return {
        m_center,
        QPointF(m_center.x() + m_radius, m_center.y()),
        QPointF(m_center.x() - m_radius, m_center.y()),
        QPointF(m_center.x(), m_center.y() + m_radius),
        QPointF(m_center.x(), m_center.y() - m_radius)
    };
}

bool CircleEntity::hitTest(const QPointF& worldPt, double tolerance) const {
    double dist = std::hypot(worldPt.x() - m_center.x(), worldPt.y() - m_center.y());
    return std::abs(dist - m_radius) <= tolerance;
}

QJsonObject CircleEntity::toJson() const {
    QJsonObject obj = CadEntity::toJson();
    obj["cx"] = m_center.x();
    obj["cy"] = m_center.y();
    obj["radius"] = m_radius;
    return obj;
}

bool CircleEntity::fromJson(const QJsonObject& json) {
    if (!CadEntity::fromJson(json)) return false;
    if (!json.contains("cx") || !json.contains("cy") || !json.contains("radius")) {
        return false;
    }
    m_center = QPointF(json["cx"].toDouble(), json["cy"].toDouble());
    m_radius = std::abs(json["radius"].toDouble());
    return true;
}
