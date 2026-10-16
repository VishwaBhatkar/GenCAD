#include "geometry/PolylineEntity.h"
#include <QJsonArray>
#include <algorithm>
#include <cmath>

static bool pointNearSegment(const QPointF& pt, const QPointF& a, const QPointF& b, double tol) {
    double dx = b.x() - a.x();
    double dy = b.y() - a.y();
    double lenSq = dx * dx + dy * dy;
    if (lenSq < 1e-12) {
        return std::hypot(pt.x() - a.x(), pt.y() - a.y()) <= tol;
    }
    double t = ((pt.x() - a.x()) * dx + (pt.y() - a.y()) * dy) / lenSq;
    t = std::max(0.0, std::min(1.0, t));
    double px = a.x() + t * dx;
    double py = a.y() + t * dy;
    return std::hypot(pt.x() - px, pt.y() - py) <= tol;
}

PolylineEntity::PolylineEntity()
    : CadEntity(EntityType::Polyline)
{
}

PolylineEntity::PolylineEntity(const std::vector<QPointF>& points, bool closed)
    : CadEntity(EntityType::Polyline)
    , m_points(points)
    , m_closed(closed)
{
}

PolylineEntity::PolylineEntity(const QUuid& id, const std::vector<QPointF>& points, bool closed)
    : CadEntity(EntityType::Polyline, id)
    , m_points(points)
    , m_closed(closed)
{
}

double PolylineEntity::totalLength() const {
    if (m_points.size() < 2) return 0.0;
    double sum = 0.0;
    for (size_t i = 1; i < m_points.size(); ++i) {
        sum += std::hypot(m_points[i].x() - m_points[i-1].x(),
                          m_points[i].y() - m_points[i-1].y());
    }
    if (m_closed && m_points.size() > 2) {
        sum += std::hypot(m_points.front().x() - m_points.back().x(),
                          m_points.front().y() - m_points.back().y());
    }
    return sum;
}

double PolylineEntity::area() const {
    if (!m_closed || m_points.size() < 3) return 0.0;
    double sum = 0.0;
    size_t n = m_points.size();
    for (size_t i = 0; i < n; ++i) {
        const QPointF& p1 = m_points[i];
        const QPointF& p2 = m_points[(i + 1) % n];
        sum += (p1.x() * p2.y() - p2.x() * p1.y());
    }
    return std::abs(sum) * 0.5;
}

std::shared_ptr<CadEntity> PolylineEntity::clone() const {
    auto copy = std::make_shared<PolylineEntity>(m_id, m_points, m_closed);
    copy->setColor(m_color);
    copy->setStrokeWidth(m_strokeWidth);
    copy->setSelected(m_selected);
    copy->setLayer(m_layer);
    return copy;
}

QRectF PolylineEntity::boundingRect() const {
    if (m_points.empty()) return QRectF();
    qreal minX = m_points.front().x();
    qreal maxX = m_points.front().x();
    qreal minY = m_points.front().y();
    qreal maxY = m_points.front().y();
    for (size_t i = 1; i < m_points.size(); ++i) {
        minX = std::min(minX, m_points[i].x());
        maxX = std::max(maxX, m_points[i].x());
        minY = std::min(minY, m_points[i].y());
        maxY = std::max(maxY, m_points[i].y());
    }
    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY));
}

void PolylineEntity::move(double deltaX, double deltaY) {
    for (auto& pt : m_points) {
        pt.rx() += deltaX;
        pt.ry() += deltaY;
    }
}

void PolylineEntity::rotate(const QPointF& origin, double angleRad) {
    for (auto& pt : m_points) {
        pt = rotatePoint(pt, origin, angleRad);
    }
}

void PolylineEntity::mirror(const QPointF& p1, const QPointF& p2) {
    for (auto& pt : m_points) {
        pt = mirrorPoint(pt, p1, p2);
    }
}

bool PolylineEntity::hitTest(const QPointF& worldPt, double tolerance) const {
    if (m_points.size() < 2) return false;
    for (size_t i = 1; i < m_points.size(); ++i) {
        if (pointNearSegment(worldPt, m_points[i-1], m_points[i], tolerance)) {
            return true;
        }
    }
    if (m_closed && m_points.size() > 2) {
        if (pointNearSegment(worldPt, m_points.back(), m_points.front(), tolerance)) {
            return true;
        }
    }
    return false;
}

std::vector<QPointF> PolylineEntity::snapPoints() const {
    std::vector<QPointF> snaps;
    for (size_t i = 0; i < m_points.size(); ++i) {
        snaps.push_back(m_points[i]);
        if (i > 0) {
            snaps.push_back(QPointF((m_points[i-1].x() + m_points[i].x()) * 0.5,
                                    (m_points[i-1].y() + m_points[i].y()) * 0.5));
        }
    }
    if (m_closed && m_points.size() > 2) {
        snaps.push_back(QPointF((m_points.back().x() + m_points.front().x()) * 0.5,
                                (m_points.back().y() + m_points.front().y()) * 0.5));
    }
    return snaps;
}

QJsonObject PolylineEntity::toJson() const {
    QJsonObject obj = CadEntity::toJson();
    obj["closed"] = m_closed;
    QJsonArray ptsArray;
    for (const auto& pt : m_points) {
        QJsonObject pObj;
        pObj["x"] = pt.x();
        pObj["y"] = pt.y();
        ptsArray.append(pObj);
    }
    obj["points"] = ptsArray;
    return obj;
}

bool PolylineEntity::fromJson(const QJsonObject& json) {
    if (!CadEntity::fromJson(json)) return false;
    m_closed = json["closed"].toBool(false);
    m_points.clear();
    if (json.contains("points") && json["points"].isArray()) {
        QJsonArray pts = json["points"].toArray();
        for (const auto& v : pts) {
            if (v.isObject()) {
                QJsonObject pObj = v.toObject();
                m_points.emplace_back(pObj["x"].toDouble(), pObj["y"].toDouble());
            }
        }
    }
    return true;
}
