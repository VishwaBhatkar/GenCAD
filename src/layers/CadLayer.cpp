#include "layers/CadLayer.h"

CadLayer::CadLayer()
    : m_name(QStringLiteral("0"))
    , m_color(QColor(220, 220, 220))
    , m_lineWeight(1.0)
    , m_visible(true)
    , m_locked(false)
{
}

CadLayer::CadLayer(const QString& name, const QColor& color, double lineWeight)
    : m_name(name)
    , m_color(color)
    , m_lineWeight(lineWeight)
    , m_visible(true)
    , m_locked(false)
{
}

QJsonObject CadLayer::toJson() const {
    QJsonObject obj;
    obj["name"] = m_name;
    obj["color"] = m_color.name();
    obj["lineWeight"] = m_lineWeight;
    obj["visible"] = m_visible;
    obj["locked"] = m_locked;
    return obj;
}

bool CadLayer::fromJson(const QJsonObject& json) {
    if (!json.contains("name")) return false;
    m_name = json["name"].toString();
    if (json.contains("color")) m_color = QColor(json["color"].toString());
    if (json.contains("lineWeight")) m_lineWeight = json["lineWeight"].toDouble(1.0);
    if (json.contains("visible")) m_visible = json["visible"].toBool(true);
    if (json.contains("locked")) m_locked = json["locked"].toBool(false);
    return true;
}
