#pragma once

#include "geometry/CadEntity.h"
#include <array>

class RectangleEntity : public CadEntity {
public:
    RectangleEntity();
    RectangleEntity(const QPointF& topLeft, double width, double height);
    RectangleEntity(const QUuid& id, const QPointF& topLeft, double width, double height);
    RectangleEntity(const QPointF& p1, const QPointF& p2);
    RectangleEntity(const QUuid& id, const std::array<QPointF, 4>& corners);

    QPointF topLeft() const { return m_corners[0]; }
    void setTopLeft(const QPointF& pt);

    double width() const;
    void setWidth(double w);

    double height() const;
    void setHeight(double h);

    const std::array<QPointF, 4>& corners() const { return m_corners; }

    double area() const override { return width() * height(); }
    double perimeter() const override { return 2.0 * (width() + height()); }
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
    void initFromTopLeft(const QPointF& topLeft, double width, double height);

    std::array<QPointF, 4> m_corners;
};
