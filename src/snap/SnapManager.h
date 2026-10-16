#pragma once

#include <QObject>
#include <QPointF>
#include <QString>
#include <vector>
#include <memory>

class CadDocument;
class CadEntity;
class QPainter;

enum class SnapType {
    None          = 0,
    Endpoint      = 1 << 0,
    Midpoint      = 1 << 1,
    Center        = 1 << 2,
    Quadrant      = 1 << 3,
    Grid          = 1 << 4,
    Intersection  = 1 << 5,
    Perpendicular = 1 << 6,
    Tangent       = 1 << 7,
    Nearest       = 1 << 8,
    Polar         = 1 << 9,
    AllObject     = Endpoint | Midpoint | Center | Quadrant | Intersection | Perpendicular | Tangent
};

inline SnapType operator|(SnapType a, SnapType b) {
    return static_cast<SnapType>(static_cast<int>(a) | static_cast<int>(b));
}

inline SnapType operator&(SnapType a, SnapType b) {
    return static_cast<SnapType>(static_cast<int>(a) & static_cast<int>(b));
}

struct SnapResult {
    bool snapped{false};
    QPointF point{0.0, 0.0};
    SnapType type{SnapType::None};
    QString description;

    // Polar tracking guide
    bool hasPolarLine{false};
    QPointF polarStart;
    QPointF polarEnd;
    double polarAngleDeg{0.0};
};

class SnapManager : public QObject {
    Q_OBJECT

public:
    explicit SnapManager(QObject* parent = nullptr);
    ~SnapManager() override = default;

    bool isSnapEnabled() const { return m_snapEnabled; }
    void setSnapEnabled(bool enabled);

    bool isGridSnapEnabled() const { return m_gridSnapEnabled; }
    void setGridSnapEnabled(bool enabled);

    bool isPolarEnabled() const { return m_polarEnabled; }
    void setPolarEnabled(bool enabled);
    bool isPolarTrackingEnabled() const { return isPolarEnabled(); }
    void setPolarTrackingEnabled(bool enabled) { setPolarEnabled(enabled); }

    double polarAngleIncrement() const { return m_polarAngleIncrement; }
    void setPolarAngleIncrement(double deg);

    void setBasePoint(const QPointF& pt, bool active = true);
    void clearBasePoint();
    bool hasBasePoint() const { return m_hasBasePoint; }
    QPointF basePoint() const { return m_basePoint; }

    int enabledModes() const { return m_enabledModes; }
    void setEnabledModes(int modes) { m_enabledModes = modes; }
    void setModeEnabled(SnapType type, bool enabled);
    bool isModeEnabled(SnapType type) const;

    double snapTolerance() const { return m_snapTolerance; }
    void setSnapTolerance(double tolerance) { m_snapTolerance = tolerance; }

    double gridSize() const { return m_gridSize; }
    void setGridSize(double size) { m_gridSize = size; }

    SnapResult findSnap(const QPointF& scenePos, const CadDocument* doc, double snapRadius) const;

    static void drawSnapMarker(QPainter* painter, const SnapResult& snap, double zoomFactor = 1.0);

signals:
    void snapSettingsChanged();
    void polarToggled(bool enabled);

private:
    bool m_snapEnabled{true};
    bool m_gridSnapEnabled{false};
    bool m_polarEnabled{false};
    double m_polarAngleIncrement{45.0};
    QPointF m_basePoint{0.0, 0.0};
    bool m_hasBasePoint{false};
    int m_enabledModes{static_cast<int>(SnapType::AllObject)};
    double m_snapTolerance{15.0}; // In screen pixels
    double m_gridSize{10.0};
};
