#include "geometry/TextEntity.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

TextEntity::TextEntity()
    : CadEntity(EntityType::Text)
    , m_position(0.0, 0.0)
    , m_text(QStringLiteral("Text"))
    , m_height(10.0)
    , m_rotationDeg(0.0)
{
}

TextEntity::TextEntity(const QPointF& pos, const QString& text, double height, double rotationDeg)
    : CadEntity(EntityType::Text)
    , m_position(pos)
    , m_text(text)
    , m_height(std::abs(height))
    , m_rotationDeg(rotationDeg)
{
}

TextEntity::TextEntity(const QUuid& id, const QPointF& pos, const QString& text, double height, double rotationDeg)
    : CadEntity(EntityType::Text, id)
    , m_position(pos)
    , m_text(text)
    , m_height(std::abs(height))
    , m_rotationDeg(rotationDeg)
{
}

std::shared_ptr<CadEntity> TextEntity::clone() const {
    auto copy = std::make_shared<TextEntity>(m_id, m_position, m_text, m_height, m_rotationDeg);
    copy->setColor(m_color);
    copy->setStrokeWidth(m_strokeWidth);
    copy->setSelected(m_selected);
    copy->setLayer(m_layer);
    return copy;
}

QRectF TextEntity::boundingRect() const {
    double estimatedWidth = std::max(10.0, m_text.length() * m_height * 0.65);
    return QRectF(m_position.x(), m_position.y() - m_height, estimatedWidth, m_height * 1.2);
}

void TextEntity::move(double deltaX, double deltaY) {
    m_position.rx() += deltaX;
    m_position.ry() += deltaY;
}

void TextEntity::rotate(const QPointF& origin, double angleRad) {
    m_position = rotatePoint(m_position, origin, angleRad);
    m_rotationDeg += angleRad * 180.0 / M_PI;
    while (m_rotationDeg >= 360.0) m_rotationDeg -= 360.0;
    while (m_rotationDeg < 0.0) m_rotationDeg += 360.0;
}

void TextEntity::mirror(const QPointF& p1, const QPointF& p2) {
    m_position = mirrorPoint(m_position, p1, p2);
}

bool TextEntity::hitTest(const QPointF& worldPt, double tolerance) const {
    QRectF box = boundingRect().adjusted(-tolerance, -tolerance, tolerance, tolerance);
    return box.contains(worldPt);
}

std::vector<QPointF> TextEntity::snapPoints() const {
    return { m_position };
}

QJsonObject TextEntity::toJson() const {
    QJsonObject obj = CadEntity::toJson();
    obj["x"] = m_position.x();
    obj["y"] = m_position.y();
    obj["text"] = m_text;
    obj["height"] = m_height;
    obj["rotation"] = m_rotationDeg;
    return obj;
}

bool TextEntity::fromJson(const QJsonObject& json) {
    if (!CadEntity::fromJson(json)) return false;
    if (!json.contains("x") || !json.contains("y") || !json.contains("text")) {
        return false;
    }
    m_position = QPointF(json["x"].toDouble(), json["y"].toDouble());
    m_text = json["text"].toString();
    m_height = std::abs(json["height"].toDouble(10.0));
    m_rotationDeg = json["rotation"].toDouble(0.0);
    return true;
}
