#include "geometry/ArcEntity.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double degToRad(double deg) { return deg * M_PI / 180.0; }
static double radToDeg(double rad) { return rad * 180.0 / M_PI; }

static double normalizeDeg(double deg) {
    while (deg < 0.0) deg += 360.0;
    while (deg >= 360.0) deg -= 360.0;
    return deg;
}

ArcEntity::ArcEntity()
    : CadEntity(EntityType::Arc)
    , m_center(0.0, 0.0)
    , m_radius(0.0)
    , m_startAngle(0.0)
    , m_sweepAngle(90.0)
{
}

ArcEntity::ArcEntity(const QPointF& center, double radius, double startAngleDeg, double sweepAngleDeg)
    : CadEntity(EntityType::Arc)
    , m_center(center)
    , m_radius(std::abs(radius))
    , m_startAngle(startAngleDeg)
    , m_sweepAngle(sweepAngleDeg)
{
}

ArcEntity::ArcEntity(const QUuid& id, const QPointF& center, double radius, double startAngleDeg, double sweepAngleDeg)
    : CadEntity(EntityType::Arc, id)
    , m_center(center)
    , m_radius(std::abs(radius))
    , m_startAngle(startAngleDeg)
    , m_sweepAngle(sweepAngleDeg)
{
}

QPointF ArcEntity::startPoint() const {
    double rad = degToRad(m_startAngle);
    return QPointF(m_center.x() + m_radius * std::cos(rad),
                   m_center.y() + m_radius * std::sin(rad));
}

QPointF ArcEntity::endPoint() const {
    double rad = degToRad(m_startAngle + m_sweepAngle);
    return QPointF(m_center.x() + m_radius * std::cos(rad),
                   m_center.y() + m_radius * std::sin(rad));
}

QPointF ArcEntity::midPoint() const {
    double rad = degToRad(m_startAngle + m_sweepAngle * 0.5);
    return QPointF(m_center.x() + m_radius * std::cos(rad),
                   m_center.y() + m_radius * std::sin(rad));
}

double ArcEntity::arcLength() const {
    return std::abs(degToRad(m_sweepAngle)) * m_radius;
}

std::shared_ptr<CadEntity> ArcEntity::clone() const {
    auto copy = std::make_shared<ArcEntity>(m_id, m_center, m_radius, m_startAngle, m_sweepAngle);
    copy->setColor(m_color);
    copy->setStrokeWidth(m_strokeWidth);
    copy->setSelected(m_selected);
    copy->setLayer(m_layer);
    return copy;
}

QRectF ArcEntity::boundingRect() const {
    return QRectF(m_center.x() - m_radius,
                  m_center.y() - m_radius,
                  2.0 * m_radius,
                  2.0 * m_radius);
}

void ArcEntity::move(double deltaX, double deltaY) {
    m_center.rx() += deltaX;
    m_center.ry() += deltaY;
}

void ArcEntity::rotate(const QPointF& origin, double angleRad) {
    m_center = rotatePoint(m_center, origin, angleRad);
    m_startAngle += radToDeg(angleRad);
    m_startAngle = normalizeDeg(m_startAngle);
}

void ArcEntity::mirror(const QPointF& p1, const QPointF& p2) {
    QPointF sp = mirrorPoint(startPoint(), p1, p2);
    QPointF ep = mirrorPoint(endPoint(), p1, p2);
    m_center = mirrorPoint(m_center, p1, p2);

    double a1 = normalizeDeg(radToDeg(std::atan2(sp.y() - m_center.y(), sp.x() - m_center.x())));
    double a2 = normalizeDeg(radToDeg(std::atan2(ep.y() - m_center.y(), ep.x() - m_center.x())));

    m_startAngle = a2;
    double diff = a1 - a2;
    if (diff < 0.0) diff += 360.0;
    m_sweepAngle = diff;
}

bool ArcEntity::hitTest(const QPointF& worldPt, double tolerance) const {
    double dist = std::hypot(worldPt.x() - m_center.x(), worldPt.y() - m_center.y());
    if (std::abs(dist - m_radius) > tolerance) {
        return false;
    }

    double ptAngle = normalizeDeg(radToDeg(std::atan2(worldPt.y() - m_center.y(), worldPt.x() - m_center.x())));
    double normStart = normalizeDeg(m_startAngle);
    double diff = ptAngle - normStart;
    if (diff < 0.0) diff += 360.0;

    double sweep = m_sweepAngle;
    if (sweep < 0) {
        return diff >= (360.0 + sweep);
    }
    return diff <= sweep;
}

std::vector<QPointF> ArcEntity::snapPoints() const {
    return {
        m_center,
        startPoint(),
        midPoint(),
        endPoint()
    };
}

QJsonObject ArcEntity::toJson() const {
    QJsonObject obj = CadEntity::toJson();
    obj["cx"] = m_center.x();
    obj["cy"] = m_center.y();
    obj["radius"] = m_radius;
    obj["startAngle"] = m_startAngle;
    obj["sweepAngle"] = m_sweepAngle;
    return obj;
}

bool ArcEntity::fromJson(const QJsonObject& json) {
    if (!CadEntity::fromJson(json)) return false;
    if (!json.contains("cx") || !json.contains("cy") ||
        !json.contains("radius") || !json.contains("startAngle") ||
        !json.contains("sweepAngle")) {
        return false;
    }
    m_center = QPointF(json["cx"].toDouble(), json["cy"].toDouble());
    m_radius = std::abs(json["radius"].toDouble());
    m_startAngle = json["startAngle"].toDouble();
    m_sweepAngle = json["sweepAngle"].toDouble();
    return true;
}
