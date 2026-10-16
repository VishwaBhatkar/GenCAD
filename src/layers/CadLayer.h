#pragma once

#include <QString>
#include <QColor>
#include <QJsonObject>

class CadLayer {
public:
    CadLayer();
    explicit CadLayer(const QString& name, const QColor& color = QColor(220, 220, 220), double lineWeight = 1.0);

    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    QColor color() const { return m_color; }
    void setColor(const QColor& color) { m_color = color; }

    double lineWeight() const { return m_lineWeight; }
    void setLineWeight(double weight) { m_lineWeight = weight; }

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    bool isLocked() const { return m_locked; }
    void setLocked(bool locked) { m_locked = locked; }

    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);

private:
    QString m_name{QStringLiteral("0")};
    QColor m_color{QColor(220, 220, 220)};
    double m_lineWeight{1.0};
    bool m_visible{true};
    bool m_locked{false};
};
