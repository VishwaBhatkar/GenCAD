#pragma once

#include "geometry/CadEntity.h"

class TextEntity : public CadEntity {
public:
    TextEntity();
    TextEntity(const QPointF& pos, const QString& text, double height = 10.0, double rotationDeg = 0.0);
    TextEntity(const QUuid& id, const QPointF& pos, const QString& text, double height = 10.0, double rotationDeg = 0.0);

    QPointF position() const { return m_position; }
    void setPosition(const QPointF& pos) { m_position = pos; }

    QString text() const { return m_text; }
    void setText(const QString& text) { m_text = text; }

    double height() const { return m_height; }
    void setHeight(double h) { m_height = std::abs(h); }

    double rotationDeg() const { return m_rotationDeg; }
    void setRotationDeg(double deg) { m_rotationDeg = deg; }

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
    QPointF m_position;
    QString m_text;
    double m_height{10.0};
    double m_rotationDeg{0.0};
};
