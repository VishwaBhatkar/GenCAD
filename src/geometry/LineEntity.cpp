#include "geometry/LineEntity.h"
#include <cmath>
#include <algorithm>

LineEntity::LineEntity()
    : CadEntity(EntityType::Line)
    , m_p1(0.0, 0.0)
    , m_p2(0.0, 0.0)
{
}

LineEntity::LineEntity(const QPointF& p1, const QPointF& p2)
    : CadEntity(EntityType::Line)
    , m_p1(p1)
    , m_p2(p2)
{
}

LineEntity::LineEntity(const QUuid& id, const QPointF& p1, const QPointF& p2)
    : CadEntity(EntityType::Line, id)
    , m_p1(p1)
    , m_p2(p2)
{
}

double LineEntity::length() const {
    double dx = m_p2.x() - m_p1.x();
    double dy = m_p2.y() - m_p1.y();
    return std::hypot(dx, dy);
}

double LineEntity::angleDeg() const {
    double dx = m_p2.x() - m_p1.x();
    double dy = m_p2.y() - m_p1.y();
    double deg = std::atan2(dy, dx) * 180.0 / 3.14159265358979323846;
    if (deg < 0.0) deg += 360.0;
    return deg;
}

std::shared_ptr<CadEntity> LineEntity::clone() const {
    auto copy = std::make_shared<LineEntity>(m_id, m_p1, m_p2);
    copy->setColor(m_color);
    copy->setStrokeWidth(m_strokeWidth);
    copy->setSelected(m_selected);
    copy->setLayer(m_layer);
    return copy;
}

QRectF LineEntity::boundingRect() const {
    qreal minX = std::min(m_p1.x(), m_p2.x());
    qreal minY = std::min(m_p1.y(), m_p2.y());
    qreal maxX = std::max(m_p1.x(), m_p2.x());
    qreal maxY = std::max(m_p1.y(), m_p2.y());
    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY));
}

void LineEntity::move(double deltaX, double deltaY) {
    m_p1.rx() += deltaX;
    m_p1.ry() += deltaY;
    m_p2.rx() += deltaX;
    m_p2.ry() += deltaY;
}

void LineEntity::rotate(const QPointF& origin, double angleRad) {
    m_p1 = rotatePoint(m_p1, origin, angleRad);
    m_p2 = rotatePoint(m_p2, origin, angleRad);
}

void LineEntity::mirror(const QPointF& p1, const QPointF& p2) {
    m_p1 = mirrorPoint(m_p1, p1, p2);
    m_p2 = mirrorPoint(m_p2, p1, p2);
}

std::vector<QPointF> LineEntity::snapPoints() const {
    return {
        m_p1,
        QPointF((m_p1.x() + m_p2.x()) * 0.5, (m_p1.y() + m_p2.y()) * 0.5),
        m_p2
    };
}

bool LineEntity::hitTest(const QPointF& worldPt, double tolerance) const {
    double dx = m_p2.x() - m_p1.x();
    double dy = m_p2.y() - m_p1.y();
    double lenSq = dx * dx + dy * dy;

    if (lenSq < 1e-12) {
        double distSq = (worldPt.x() - m_p1.x()) * (worldPt.x() - m_p1.x()) +
                        (worldPt.y() - m_p1.y()) * (worldPt.y() - m_p1.y());
        return distSq <= (tolerance * tolerance);
    }

    double t = ((worldPt.x() - m_p1.x()) * dx + (worldPt.y() - m_p1.y()) * dy) / lenSq;
    t = std::max(0.0, std::min(1.0, t));

    double projX = m_p1.x() + t * dx;
    double projY = m_p1.y() + t * dy;

    double distSq = (worldPt.x() - projX) * (worldPt.x() - projX) +
                    (worldPt.y() - projY) * (worldPt.y() - projY);

    return distSq <= (tolerance * tolerance);
}

QJsonObject LineEntity::toJson() const {
    QJsonObject obj = CadEntity::toJson();
    obj["x1"] = m_p1.x();
    obj["y1"] = m_p1.y();
    obj["x2"] = m_p2.x();
    obj["y2"] = m_p2.y();
    return obj;
}

bool LineEntity::fromJson(const QJsonObject& json) {
    if (!CadEntity::fromJson(json)) return false;
    if (json.contains("x1") && json.contains("y1") &&
        json.contains("x2") && json.contains("y2")) {
        m_p1 = QPointF(json["x1"].toDouble(), json["y1"].toDouble());
        m_p2 = QPointF(json["x2"].toDouble(), json["y2"].toDouble());
        return true;
    }
    if (json.contains("p1") && json.contains("p2")) {
        QJsonObject p1Obj = json["p1"].toObject();
        QJsonObject p2Obj = json["p2"].toObject();
        m_p1 = QPointF(p1Obj["x"].toDouble(), p1Obj["y"].toDouble());
        m_p2 = QPointF(p2Obj["x"].toDouble(), p2Obj["y"].toDouble());
        return true;
    }
    return false;
}
