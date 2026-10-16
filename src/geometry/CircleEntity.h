#pragma once

#include "geometry/CadEntity.h"

class CircleEntity : public CadEntity {
public:
    CircleEntity();
    CircleEntity(const QPointF& center, double radius);
    CircleEntity(const QUuid& id, const QPointF& center, double radius);

    QPointF center() const { return m_center; }
    void setCenter(const QPointF& center) { m_center = center; }

    double radius() const { return m_radius; }
    void setRadius(double radius) { m_radius = radius; }

    double diameter() const { return 2.0 * m_radius; }
    double area() const override;
    double circumference() const;
    double perimeter() const override { return circumference(); }
    bool isClosed() const override { return true; }

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
};
