#pragma once

#include "geometry/CadEntity.h"
#include <vector>

class PolylineEntity : public CadEntity {
public:
    PolylineEntity();
    explicit PolylineEntity(const std::vector<QPointF>& points, bool closed = false);
    PolylineEntity(const QUuid& id, const std::vector<QPointF>& points, bool closed = false);

    const std::vector<QPointF>& points() const { return m_points; }
    void setPoints(const std::vector<QPointF>& pts) { m_points = pts; }

    void addPoint(const QPointF& pt) { m_points.push_back(pt); }
    size_t pointCount() const { return m_points.size(); }
    QPointF pointAt(size_t index) const { return m_points.at(index); }

    bool isClosed() const override { return m_closed; }
    void setClosed(bool closed) { m_closed = closed; }

    double totalLength() const;
    double area() const override;
    double perimeter() const override { return totalLength(); }

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
    std::vector<QPointF> m_points;
    bool m_closed{false};
};
