#pragma once

#include "geometry/CadEntity.h"

class LineEntity : public CadEntity {
public:
    LineEntity();
    LineEntity(const QPointF& p1, const QPointF& p2);
    LineEntity(const QUuid& id, const QPointF& p1, const QPointF& p2);

    QPointF p1() const { return m_p1; }
    void setP1(const QPointF& p1) { m_p1 = p1; }

    QPointF p2() const { return m_p2; }
    void setP2(const QPointF& p2) { m_p2 = p2; }

    QPointF startPoint() const { return m_p1; }
    QPointF endPoint() const { return m_p2; }

    double length() const;
    double angleDeg() const;
    QPointF midPoint() const { return (m_p1 + m_p2) * 0.5; }
    double area() const override { return 0.0; }
    double perimeter() const override { return length(); }
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
    QPointF m_p1;
    QPointF m_p2;
};
