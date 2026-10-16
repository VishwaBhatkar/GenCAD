#include "geometry/CadEntity.h"
#include <cmath>
#include <algorithm>

CadEntity::CadEntity(EntityType type)
    : m_type(type)
    , m_id(QUuid::createUuid())
{
}

CadEntity::CadEntity(EntityType type, const QUuid& id)
    : m_type(type)
    , m_id(id)
{
}

QJsonObject CadEntity::toJson() const {
    QJsonObject obj;
    obj["id"] = m_id.toString(QUuid::WithoutBraces);
    obj["type"] = entityTypeToString(m_type);
    obj["layer"] = m_layer;
    obj["color"] = m_color.name();
    obj["strokeWidth"] = m_strokeWidth;
    return obj;
}

bool CadEntity::fromJson(const QJsonObject& json) {
    if (json.contains("id")) {
        m_id = QUuid::fromString(json["id"].toString());
        if (m_id.isNull()) {
            m_id = QUuid::createUuid();
        }
    }
    if (json.contains("layer")) {
        m_layer = json["layer"].toString();
    }
    if (json.contains("color")) {
        m_color = QColor(json["color"].toString());
    }
    if (json.contains("strokeWidth")) {
        m_strokeWidth = json["strokeWidth"].toDouble(1.5);
    }
    return true;
}

QString CadEntity::entityTypeToString(EntityType type) {
    switch (type) {
        case EntityType::Line: return QStringLiteral("line");
        case EntityType::Circle: return QStringLiteral("circle");
        case EntityType::Rectangle: return QStringLiteral("rectangle");
        case EntityType::Arc: return QStringLiteral("arc");
        case EntityType::Polyline: return QStringLiteral("polyline");
        case EntityType::Text: return QStringLiteral("text");
        case EntityType::Dimension: return QStringLiteral("dimension");
        case EntityType::Polygon: return QStringLiteral("polygon");
        case EntityType::Ellipse: return QStringLiteral("ellipse");
    }
    return QStringLiteral("unknown");
}

EntityType CadEntity::stringToEntityType(const QString& str, bool* ok) {
    if (ok) *ok = true;
    if (str.compare(QStringLiteral("line"), Qt::CaseInsensitive) == 0) return EntityType::Line;
    if (str.compare(QStringLiteral("circle"), Qt::CaseInsensitive) == 0) return EntityType::Circle;
    if (str.compare(QStringLiteral("rectangle"), Qt::CaseInsensitive) == 0) return EntityType::Rectangle;
    if (str.compare(QStringLiteral("arc"), Qt::CaseInsensitive) == 0) return EntityType::Arc;
    if (str.compare(QStringLiteral("polyline"), Qt::CaseInsensitive) == 0) return EntityType::Polyline;
    if (str.compare(QStringLiteral("text"), Qt::CaseInsensitive) == 0) return EntityType::Text;
    if (str.compare(QStringLiteral("dimension"), Qt::CaseInsensitive) == 0) return EntityType::Dimension;
    if (str.compare(QStringLiteral("polygon"), Qt::CaseInsensitive) == 0) return EntityType::Polygon;
    if (str.compare(QStringLiteral("ellipse"), Qt::CaseInsensitive) == 0) return EntityType::Ellipse;
    if (ok) *ok = false;
    return EntityType::Line;
}

QPointF CadEntity::rotatePoint(const QPointF& pt, const QPointF& origin, double angleRad) {
    double cosA = std::cos(angleRad);
    double sinA = std::sin(angleRad);
    double dx = pt.x() - origin.x();
    double dy = pt.y() - origin.y();
    return QPointF(origin.x() + dx * cosA - dy * sinA,
                   origin.y() + dx * sinA + dy * cosA);
}

QPointF CadEntity::mirrorPoint(const QPointF& pt, const QPointF& p1, const QPointF& p2) {
    double dx = p2.x() - p1.x();
    double dy = p2.y() - p1.y();
    double lenSq = dx * dx + dy * dy;
    if (lenSq < 1e-12) {
        return pt;
    }
    double t = ((pt.x() - p1.x()) * dx + (pt.y() - p1.y()) * dy) / lenSq;
    double projX = p1.x() + t * dx;
    double projY = p1.y() + t * dy;
    return QPointF(2.0 * projX - pt.x(), 2.0 * projY - pt.y());
}
