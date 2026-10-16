#pragma once

#include "geometry/CadEntity.h"

class ArcEntity : public CadEntity {
public:
    ArcEntity();
    ArcEntity(const QPointF& center, double radius, double startAngleDeg, double sweepAngleDeg);
    ArcEntity(const QUuid& id, const QPointF& center, double radius, double startAngleDeg, double sweepAngleDeg);

    QPointF center() const { return m_center; }
    void setCenter(const QPointF& center) { m_center = center; }

    double radius() const { return m_radius; }
    void setRadius(double r) { m_radius = std::abs(r); }

    double startAngle() const { return m_startAngle; }
    void setStartAngle(double a) { m_startAngle = a; }

    double sweepAngle() const { return m_sweepAngle; }
    void setSweepAngle(double a) { m_sweepAngle = a; }

    double endAngle() const { return m_startAngle + m_sweepAngle; }

    QPointF startPoint() const;
    QPointF endPoint() const;
    QPointF midPoint() const;
    double arcLength() const;
    double area() const override { return 0.5 * m_radius * m_radius * (std::abs(m_sweepAngle) * 3.14159265358979323846 / 180.0); }
    double perimeter() const override { return arcLength(); }
    bool isClosed() const override { return false; }

    std::shared_ptr<CadEntity> clone() const override;
    QRectF boundingRect() const override;
    void move(double deltaX, double deltaY) override;
    void rotate(const QPointF& origin, double angleRad) override;
    void mirror(const QPointF& p1, const QPointF& p2) override;
    bool hitTest(const QPointF& worldPt, double tolerance) const override;
    std::vector<QPointF> snapPoints() const override;

    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;

private:
    QPointF m_center;
    double m_radius{0.0};
    double m_startAngle{0.0};  // In degrees
    double m_sweepAngle{0.0};  // In degrees
};
