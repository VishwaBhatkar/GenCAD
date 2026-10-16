#pragma once

#include <QUuid>
#include <QPointF>
#include <QRectF>
#include <QColor>
#include <QJsonObject>
#include <QString>
#include <vector>
#include <memory>

enum class EntityType {
    Line,
    Circle,
    Rectangle,
    Arc,
    Polyline,
    Text,
    Dimension,
    Polygon,
    Ellipse
};

class CadEntity {
public:
    explicit CadEntity(EntityType type);
    CadEntity(EntityType type, const QUuid& id);
    virtual ~CadEntity() = default;

    EntityType type() const { return m_type; }
    QUuid id() const { return m_id; }
    void setId(const QUuid& id) { m_id = id; }

    bool isSelected() const { return m_selected; }
    void setSelected(bool selected) { m_selected = selected; }

    QString layer() const { return m_layer; }
    void setLayer(const QString& layer) { m_layer = layer; }

    QColor color() const { return m_color; }
    void setColor(const QColor& color) { m_color = color; }

    double strokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(double width) { m_strokeWidth = width; }

    virtual std::shared_ptr<CadEntity> clone() const = 0;
    virtual QRectF boundingRect() const = 0;
    virtual void move(double deltaX, double deltaY) = 0;
    virtual void rotate(const QPointF& origin, double angleRad) = 0;
    virtual void mirror(const QPointF& p1, const QPointF& p2) = 0;
    virtual bool hitTest(const QPointF& worldPt, double tolerance) const = 0;
    virtual std::vector<QPointF> snapPoints() const { return {}; }
    virtual double area() const { return 0.0; }
    virtual double perimeter() const { return 0.0; }
    virtual bool isClosed() const { return false; }

    virtual QJsonObject toJson() const;
    virtual bool fromJson(const QJsonObject& json);

    static QString entityTypeToString(EntityType type);
    static EntityType stringToEntityType(const QString& str, bool* ok = nullptr);

    // Geometry transformation helpers
    static QPointF rotatePoint(const QPointF& pt, const QPointF& origin, double angleRad);
    static QPointF mirrorPoint(const QPointF& pt, const QPointF& p1, const QPointF& p2);

protected:
    EntityType m_type;
    QUuid m_id;
    QString m_layer;
    bool m_selected{false};
    QColor m_color{QColor(220, 220, 220)};
    double m_strokeWidth{1.5};
};
