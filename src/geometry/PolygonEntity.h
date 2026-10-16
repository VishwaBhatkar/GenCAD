#pragma once

#include "geometry/CadEntity.h"
#include <vector>

class PolygonEntity : public CadEntity {
public:
    PolygonEntity();
    PolygonEntity(const QPointF& center, int sides, double radius, double rotationDeg = 0.0);
    PolygonEntity(const QUuid& id, const QPointF& center, int sides, double radius, double rotationDeg = 0.0);

    QPointF center() const { return m_center; }
    void setCenter(const QPointF& center) { m_center = center; }

    int sides() const { return m_sides; }
    void setSides(int sides);

    double radius() const { return m_radius; }
    void setRadius(double r);

    double rotationDeg() const { return m_rotationDeg; }
    void setRotationDeg(double deg) { m_rotationDeg = deg; }

    std::vector<QPointF> vertices() const;
    double sideLength() const;
    double area() const override;
    double perimeter() const override;
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
    QPointF m_center{0.0, 0.0};
    int m_sides{6};
    double m_radius{100.0};
    double m_rotationDeg{0.0};
};
