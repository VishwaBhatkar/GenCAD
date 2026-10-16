#include "geometry/RectangleEntity.h"
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

RectangleEntity::RectangleEntity()
    : CadEntity(EntityType::Rectangle)
{
    initFromTopLeft(QPointF(0.0, 0.0), 0.0, 0.0);
}

RectangleEntity::RectangleEntity(const QPointF& topLeft, double width, double height)
    : CadEntity(EntityType::Rectangle)
{
    initFromTopLeft(topLeft, std::abs(width), std::abs(height));
}

RectangleEntity::RectangleEntity(const QUuid& id, const QPointF& topLeft, double width, double height)
    : CadEntity(EntityType::Rectangle, id)
{
    initFromTopLeft(topLeft, std::abs(width), std::abs(height));
}

RectangleEntity::RectangleEntity(const QPointF& p1, const QPointF& p2)
    : CadEntity(EntityType::Rectangle)
{
    qreal minX = std::min(p1.x(), p2.x());
    qreal minY = std::min(p1.y(), p2.y());
    initFromTopLeft(QPointF(minX, minY), std::abs(p2.x() - p1.x()), std::abs(p2.y() - p1.y()));
}

RectangleEntity::RectangleEntity(const QUuid& id, const std::array<QPointF, 4>& corners)
    : CadEntity(EntityType::Rectangle, id)
    , m_corners(corners)
{
}

void RectangleEntity::initFromTopLeft(const QPointF& topLeft, double width, double height) {
    m_corners[0] = topLeft;
    m_corners[1] = QPointF(topLeft.x() + width, topLeft.y());
    m_corners[2] = QPointF(topLeft.x() + width, topLeft.y() + height);
    m_corners[3] = QPointF(topLeft.x(), topLeft.y() + height);
}

void RectangleEntity::setTopLeft(const QPointF& pt) {
    double w = width();
    double h = height();
    initFromTopLeft(pt, w, h);
}

double RectangleEntity::width() const {
    return std::hypot(m_corners[1].x() - m_corners[0].x(), m_corners[1].y() - m_corners[0].y());
}

void RectangleEntity::setWidth(double w) {
    initFromTopLeft(m_corners[0], std::abs(w), height());
}

double RectangleEntity::height() const {
    return std::hypot(m_corners[3].x() - m_corners[0].x(), m_corners[3].y() - m_corners[0].y());
}

void RectangleEntity::setHeight(double h) {
    initFromTopLeft(m_corners[0], width(), std::abs(h));
}

std::shared_ptr<CadEntity> RectangleEntity::clone() const {
    auto copy = std::make_shared<RectangleEntity>(m_id, m_corners);
    copy->setColor(m_color);
    copy->setStrokeWidth(m_strokeWidth);
    copy->setSelected(m_selected);
    copy->setLayer(m_layer);
    return copy;
}

QRectF RectangleEntity::boundingRect() const {
    qreal minX = m_corners[0].x();
    qreal maxX = m_corners[0].x();
    qreal minY = m_corners[0].y();
    qreal maxY = m_corners[0].y();
    for (int i = 1; i < 4; ++i) {
        minX = std::min(minX, m_corners[i].x());
        maxX = std::max(maxX, m_corners[i].x());
        minY = std::min(minY, m_corners[i].y());
        maxY = std::max(maxY, m_corners[i].y());
    }
    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY));
}

void RectangleEntity::move(double deltaX, double deltaY) {
    for (auto& corner : m_corners) {
        corner.rx() += deltaX;
        corner.ry() += deltaY;
    }
}

void RectangleEntity::rotate(const QPointF& origin, double angleRad) {
    for (auto& corner : m_corners) {
        corner = rotatePoint(corner, origin, angleRad);
    }
}

void RectangleEntity::mirror(const QPointF& p1, const QPointF& p2) {
    for (auto& corner : m_corners) {
        corner = mirrorPoint(corner, p1, p2);
    }
    // Swap corners 1 and 3 to keep winding order consistent
    std::swap(m_corners[1], m_corners[3]);
}

std::vector<QPointF> RectangleEntity::snapPoints() const {
    return {
        m_corners[0],
        m_corners[1],
        m_corners[2],
        m_corners[3],
        QPointF((m_corners[0].x() + m_corners[1].x()) * 0.5, (m_corners[0].y() + m_corners[1].y()) * 0.5),
        QPointF((m_corners[1].x() + m_corners[2].x()) * 0.5, (m_corners[1].y() + m_corners[2].y()) * 0.5),
        QPointF((m_corners[2].x() + m_corners[3].x()) * 0.5, (m_corners[2].y() + m_corners[3].y()) * 0.5),
        QPointF((m_corners[3].x() + m_corners[0].x()) * 0.5, (m_corners[3].y() + m_corners[0].y()) * 0.5),
        QPointF((m_corners[0].x() + m_corners[2].x()) * 0.5, (m_corners[0].y() + m_corners[2].y()) * 0.5)
    };
}

bool RectangleEntity::hitTest(const QPointF& worldPt, double tolerance) const {
    if (pointNearSegment(worldPt, m_corners[0], m_corners[1], tolerance)) return true;
    if (pointNearSegment(worldPt, m_corners[1], m_corners[2], tolerance)) return true;
    if (pointNearSegment(worldPt, m_corners[2], m_corners[3], tolerance)) return true;
    if (pointNearSegment(worldPt, m_corners[3], m_corners[0], tolerance)) return true;
    return false;
}

QJsonObject RectangleEntity::toJson() const {
    QJsonObject obj = CadEntity::toJson();
    obj["x"] = m_corners[0].x();
    obj["y"] = m_corners[0].y();
    obj["width"] = width();
    obj["height"] = height();

    // Also store exact corners for rotated rectangles
    obj["x1"] = m_corners[1].x();
    obj["y1"] = m_corners[1].y();
    obj["x2"] = m_corners[2].x();
    obj["y2"] = m_corners[2].y();
    obj["x3"] = m_corners[3].x();
    obj["y3"] = m_corners[3].y();
    return obj;
}

bool RectangleEntity::fromJson(const QJsonObject& json) {
    if (!CadEntity::fromJson(json)) return false;
    if (json.contains("x1") && json.contains("y1") &&
        json.contains("x2") && json.contains("y2") &&
        json.contains("x3") && json.contains("y3")) {
        m_corners[0] = QPointF(json["x"].toDouble(), json["y"].toDouble());
        m_corners[1] = QPointF(json["x1"].toDouble(), json["y1"].toDouble());
        m_corners[2] = QPointF(json["x2"].toDouble(), json["y2"].toDouble());
        m_corners[3] = QPointF(json["x3"].toDouble(), json["y3"].toDouble());
    } else if (json.contains("x") && json.contains("y") &&
               json.contains("width") && json.contains("height")) {
        initFromTopLeft(QPointF(json["x"].toDouble(), json["y"].toDouble()),
                        std::abs(json["width"].toDouble()),
                        std::abs(json["height"].toDouble()));
    } else {
        return false;
    }
    return true;
}
