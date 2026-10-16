#pragma once

#include "geometry/CadEntity.h"

class EllipseEntity : public CadEntity {
public:
    EllipseEntity();
    EllipseEntity(const QPointF& center, double radiusX, double radiusY, double rotationDeg = 0.0);
    EllipseEntity(const QUuid& id, const QPointF& center, double radiusX, double radiusY, double rotationDeg = 0.0);

    QPointF center() const { return m_center; }
    void setCenter(const QPointF& center) { m_center = center; }

    double radiusX() const { return m_radiusX; }
    void setRadiusX(double rx);

    double radiusY() const { return m_radiusY; }
    void setRadiusY(double ry);

    double majorAxis() const { return 2.0 * std::max(m_radiusX, m_radiusY); }
    double minorAxis() const { return 2.0 * std::min(m_radiusX, m_radiusY); }

    double rotationDeg() const { return m_rotationDeg; }
    void setRotationDeg(double deg) { m_rotationDeg = deg; }

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
    double m_radiusX{100.0};
    double m_radiusY{50.0};
    double m_rotationDeg{0.0};
};
