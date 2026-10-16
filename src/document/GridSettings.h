#pragma once

#include <QJsonObject>

class GridSettings {
public:
    GridSettings()
        : m_visible(true)
        , m_spacing(10.0)
        , m_majorMultiplier(10)
        , m_snapToGrid(false)
        , m_snapSpacing(10.0)
    {}

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    double spacing() const { return m_spacing; }
    void setSpacing(double spacing) { if (spacing > 0.01) m_spacing = spacing; }

    int majorMultiplier() const { return m_majorMultiplier; }
    void setMajorMultiplier(int mult) { if (mult > 0) m_majorMultiplier = mult; }

    double majorSpacing() const { return m_spacing * m_majorMultiplier; }

    bool snapToGrid() const { return m_snapToGrid; }
    void setSnapToGrid(bool snap) { m_snapToGrid = snap; }

    double snapSpacing() const { return m_snapSpacing; }
    void setSnapSpacing(double spacing) { if (spacing > 0.01) m_snapSpacing = spacing; }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj[QStringLiteral("visible")] = m_visible;
        obj[QStringLiteral("spacing")] = m_spacing;
        obj[QStringLiteral("majorMultiplier")] = m_majorMultiplier;
        obj[QStringLiteral("snapToGrid")] = m_snapToGrid;
        obj[QStringLiteral("snapSpacing")] = m_snapSpacing;
        return obj;
    }

    bool fromJson(const QJsonObject& json) {
        if (json.contains(QStringLiteral("visible"))) m_visible = json[QStringLiteral("visible")].toBool(true);
        if (json.contains(QStringLiteral("spacing"))) m_spacing = json[QStringLiteral("spacing")].toDouble(10.0);
        if (json.contains(QStringLiteral("majorMultiplier"))) m_majorMultiplier = json[QStringLiteral("majorMultiplier")].toInt(10);
        if (json.contains(QStringLiteral("snapToGrid"))) m_snapToGrid = json[QStringLiteral("snapToGrid")].toBool(false);
        if (json.contains(QStringLiteral("snapSpacing"))) m_snapSpacing = json[QStringLiteral("snapSpacing")].toDouble(10.0);
        return true;
    }

private:
    bool m_visible{true};
    double m_spacing{10.0};
    int m_majorMultiplier{10};
    bool m_snapToGrid{false};
    double m_snapSpacing{10.0};
};
