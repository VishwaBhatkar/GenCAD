#include "geometry/DimensionEntity.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"
#include "geometry/ArcEntity.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DimensionEntity::DimensionEntity()
    : CadEntity(EntityType::Dimension)
    , m_dimType(DimensionType::Aligned)
    , m_p1(0.0, 0.0)
    , m_p2(0.0, 0.0)
    , m_dimPoint(0.0, 0.0)
    , m_targetFeatureIndex(0)
    , m_unitSuffix()
    , m_precision(2)
{
    m_color = QColor(0, 180, 255);
}

DimensionEntity::DimensionEntity(const QPointF& p1, const QPointF& p2, const QPointF& dimPoint, DimensionType type)
    : CadEntity(EntityType::Dimension)
    , m_dimType(type)
    , m_p1(p1)
    , m_p2(p2)
    , m_dimPoint(dimPoint)
    , m_targetFeatureIndex(0)
    , m_unitSuffix()
    , m_precision(2)
{
    m_color = QColor(0, 180, 255);
}

DimensionEntity::DimensionEntity(const QUuid& id, const QPointF& p1, const QPointF& p2, const QPointF& dimPoint, DimensionType type)
    : CadEntity(EntityType::Dimension, id)
    , m_dimType(type)
    , m_p1(p1)
    , m_p2(p2)
    , m_dimPoint(dimPoint)
    , m_targetFeatureIndex(0)
    , m_unitSuffix()
    , m_precision(2)
{
    m_color = QColor(0, 180, 255);
}

double DimensionEntity::measuredValue() const {
    switch (m_dimType) {
        case DimensionType::Aligned:
            return std::hypot(m_p2.x() - m_p1.x(), m_p2.y() - m_p1.y());
        case DimensionType::Horizontal:
            return std::abs(m_p2.x() - m_p1.x());
        case DimensionType::Vertical:
            return std::abs(m_p2.y() - m_p1.y());
        case DimensionType::Radius:
            return std::hypot(m_p2.x() - m_p1.x(), m_p2.y() - m_p1.y());
        case DimensionType::Diameter:
            return 2.0 * std::hypot(m_p2.x() - m_p1.x(), m_p2.y() - m_p1.y());
        case DimensionType::Angular: {
            double a1 = std::atan2(m_p1.y() - m_dimPoint.y(), m_p1.x() - m_dimPoint.x()) * 180.0 / M_PI;
            double a2 = std::atan2(m_p2.y() - m_dimPoint.y(), m_p2.x() - m_dimPoint.x()) * 180.0 / M_PI;
            double diff = std::abs(a2 - a1);
            if (diff > 180.0) diff = 360.0 - diff;
            return diff;
        }
        case DimensionType::ArcLength: {
            double r = std::hypot(m_p1.x() - m_dimPoint.x(), m_p1.y() - m_dimPoint.y());
            double a1 = std::atan2(m_p1.y() - m_dimPoint.y(), m_p1.x() - m_dimPoint.x());
            double a2 = std::atan2(m_p2.y() - m_dimPoint.y(), m_p2.x() - m_dimPoint.x());
            double d = std::abs(a2 - a1);
            if (d > M_PI) d = 2.0 * M_PI - d;
            return r * d;
        }
    }
    return std::hypot(m_p2.x() - m_p1.x(), m_p2.y() - m_p1.y());
}

QString DimensionEntity::dimensionText() const {
    double val = measuredValue();
    QString numStr = QString::number(val, 'f', m_precision);
    QString sfx = m_unitSuffix.isEmpty() ? QString() : (QStringLiteral(" ") + m_unitSuffix);

    switch (m_dimType) {
        case DimensionType::Aligned:
        case DimensionType::Horizontal:
        case DimensionType::Vertical:
            return numStr + sfx;
        case DimensionType::Radius:
            return QStringLiteral("R") + numStr + sfx;
        case DimensionType::Diameter:
            return QStringLiteral("Ø") + numStr + sfx;
        case DimensionType::Angular:
            return QString::number(val, 'f', std::min(m_precision, 2)) + QStringLiteral("°");
        case DimensionType::ArcLength:
            return QStringLiteral("arc ") + numStr + sfx;
    }
    return numStr + sfx;
}

bool DimensionEntity::updateFromTarget(const CadEntity* target) {
    if (!target) return false;

    if (target->type() == EntityType::Line) {
        const auto* line = static_cast<const LineEntity*>(target);
        m_p1 = line->p1();
        m_p2 = line->p2();
        return true;
    } else if (target->type() == EntityType::Rectangle) {
        const auto* rect = static_cast<const RectangleEntity*>(target);
        const auto& c = rect->corners();
        int idx = std::clamp(m_targetFeatureIndex, 0, 3);
        m_p1 = c[idx];
        m_p2 = c[(idx + 1) % 4];
        return true;
    } else if (target->type() == EntityType::Circle) {
        const auto* circle = static_cast<const CircleEntity*>(target);
        m_p1 = circle->center();
        m_p2 = circle->center() + QPointF(circle->radius(), 0.0);
        return true;
    } else if (target->type() == EntityType::Arc) {
        const auto* arc = static_cast<const ArcEntity*>(target);
        m_p1 = arc->startPoint();
        m_p2 = arc->endPoint();
        m_dimPoint = arc->center();
        return true;
    }
    return false;
}

std::shared_ptr<CadEntity> DimensionEntity::clone() const {
    auto copy = std::make_shared<DimensionEntity>(m_id, m_p1, m_p2, m_dimPoint, m_dimType);
    copy->setColor(m_color);
    copy->setStrokeWidth(m_strokeWidth);
    copy->setSelected(m_selected);
    copy->setLayer(m_layer);
    copy->setTargetEntityId(m_targetEntityId);
    copy->setTargetFeatureIndex(m_targetFeatureIndex);
    copy->setUnitSuffix(m_unitSuffix);
    copy->setPrecision(m_precision);
    return copy;
}

QRectF DimensionEntity::boundingRect() const {
    qreal minX = std::min({m_p1.x(), m_p2.x(), m_dimPoint.x()});
    qreal maxX = std::max({m_p1.x(), m_p2.x(), m_dimPoint.x()});
    qreal minY = std::min({m_p1.y(), m_p2.y(), m_dimPoint.y()});
    qreal maxY = std::max({m_p1.y(), m_p2.y(), m_dimPoint.y()});
    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).adjusted(-15, -15, 15, 15);
}

void DimensionEntity::move(double deltaX, double deltaY) {
    m_p1.rx() += deltaX;
    m_p1.ry() += deltaY;
    m_p2.rx() += deltaX;
    m_p2.ry() += deltaY;
    m_dimPoint.rx() += deltaX;
    m_dimPoint.ry() += deltaY;
}

void DimensionEntity::rotate(const QPointF& origin, double angleRad) {
    m_p1 = rotatePoint(m_p1, origin, angleRad);
    m_p2 = rotatePoint(m_p2, origin, angleRad);
    m_dimPoint = rotatePoint(m_dimPoint, origin, angleRad);
}

void DimensionEntity::mirror(const QPointF& p1, const QPointF& p2) {
    m_p1 = mirrorPoint(m_p1, p1, p2);
    m_p2 = mirrorPoint(m_p2, p1, p2);
    m_dimPoint = mirrorPoint(m_dimPoint, p1, p2);
}

bool DimensionEntity::hitTest(const QPointF& worldPt, double tolerance) const {
    return boundingRect().adjusted(-tolerance, -tolerance, tolerance, tolerance).contains(worldPt);
}

std::vector<QPointF> DimensionEntity::snapPoints() const {
    return {
        m_p1,
        m_p2,
        QPointF((m_p1.x() + m_p2.x()) * 0.5, (m_p1.y() + m_p2.y()) * 0.5),
        m_dimPoint
    };
}

QJsonObject DimensionEntity::toJson() const {
    QJsonObject obj = CadEntity::toJson();
    obj[QStringLiteral("dimType")] = dimensionTypeToString(m_dimType);
    obj[QStringLiteral("x1")] = m_p1.x();
    obj[QStringLiteral("y1")] = m_p1.y();
    obj[QStringLiteral("x2")] = m_p2.x();
    obj[QStringLiteral("y2")] = m_p2.y();
    obj[QStringLiteral("dimX")] = m_dimPoint.x();
    obj[QStringLiteral("dimY")] = m_dimPoint.y();
    if (!m_targetEntityId.isNull()) {
        obj[QStringLiteral("targetEntityId")] = m_targetEntityId.toString(QUuid::WithoutBraces);
    }
    obj[QStringLiteral("targetFeatureIndex")] = m_targetFeatureIndex;
    obj[QStringLiteral("unitSuffix")] = m_unitSuffix;
    obj[QStringLiteral("precision")] = m_precision;
    return obj;
}

bool DimensionEntity::fromJson(const QJsonObject& json) {
    if (!CadEntity::fromJson(json)) return false;
    if (json.contains(QStringLiteral("dimType"))) {
        m_dimType = stringToDimensionType(json[QStringLiteral("dimType")].toString());
    }
    m_p1.setX(json[QStringLiteral("x1")].toDouble(0.0));
    m_p1.setY(json[QStringLiteral("y1")].toDouble(0.0));
    m_p2.setX(json[QStringLiteral("x2")].toDouble(0.0));
    m_p2.setY(json[QStringLiteral("y2")].toDouble(0.0));
    m_dimPoint.setX(json[QStringLiteral("dimX")].toDouble(0.0));
    m_dimPoint.setY(json[QStringLiteral("dimY")].toDouble(0.0));
    if (json.contains(QStringLiteral("targetEntityId"))) {
        m_targetEntityId = QUuid::fromString(json[QStringLiteral("targetEntityId")].toString());
    }
    if (json.contains(QStringLiteral("targetFeatureIndex"))) {
        m_targetFeatureIndex = json[QStringLiteral("targetFeatureIndex")].toInt(0);
    }
    if (json.contains(QStringLiteral("unitSuffix"))) {
        m_unitSuffix = json[QStringLiteral("unitSuffix")].toString(QStringLiteral("mm"));
    }
    if (json.contains(QStringLiteral("precision"))) {
        m_precision = json[QStringLiteral("precision")].toInt(2);
    }
    return true;
}

QString DimensionEntity::dimensionTypeToString(DimensionType type) {
    switch (type) {
        case DimensionType::Aligned: return QStringLiteral("aligned");
        case DimensionType::Horizontal: return QStringLiteral("horizontal");
        case DimensionType::Vertical: return QStringLiteral("vertical");
        case DimensionType::Radius: return QStringLiteral("radius");
        case DimensionType::Diameter: return QStringLiteral("diameter");
        case DimensionType::Angular: return QStringLiteral("angular");
        case DimensionType::ArcLength: return QStringLiteral("arclength");
    }
    return QStringLiteral("aligned");
}

DimensionType DimensionEntity::stringToDimensionType(const QString& str, bool* ok) {
    QString s = str.trimmed().toLower();
    if (ok) *ok = true;
    if (s == QStringLiteral("aligned") || s == QStringLiteral("linear")) return DimensionType::Aligned;
    if (s == QStringLiteral("horizontal")) return DimensionType::Horizontal;
    if (s == QStringLiteral("vertical")) return DimensionType::Vertical;
    if (s == QStringLiteral("radius")) return DimensionType::Radius;
    if (s == QStringLiteral("diameter")) return DimensionType::Diameter;
    if (s == QStringLiteral("angular")) return DimensionType::Angular;
    if (s == QStringLiteral("arclength")) return DimensionType::ArcLength;

    if (ok) *ok = false;
    return DimensionType::Aligned;
}
