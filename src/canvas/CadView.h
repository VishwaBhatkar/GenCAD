#pragma once

#include <QGraphicsView>
#include <QPointF>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include "ui/CadTheme.h"

class CadScene;
class Tool;
class SnapManager;

class CadView : public QGraphicsView {
    Q_OBJECT

public:
    explicit CadView(CadScene* scene, QWidget* parent = nullptr);
    ~CadView() override;

    CadScene* cadScene() const { return m_scene; }

    void setTool(Tool* tool);
    Tool* currentTool() const { return m_currentTool; }

    // Snap & Ortho
    SnapManager* snapManager() const { return m_snapManager; }
    void setSnapManager(SnapManager* snapManager);

    bool isOrthoEnabled() const { return m_orthoEnabled; }
    void setOrthoEnabled(bool enabled);

    // Crosshair options
    void setCrosshairVisible(bool visible);
    bool isCrosshairVisible() const { return m_crosshairVisible; }

    // Navigation
    void zoomIn();
    void zoomOut();
    void zoomFit();
    void resetZoom();

    // Theme Mode
    void setThemeMode(CadTheme::ThemeMode mode);
    CadTheme::ThemeMode themeMode() const { return m_themeMode; }

    // Coordinates conversion
    QPointF screenToWorld(const QPoint& screenPt) const;
    QPoint worldToScreen(const QPointF& worldPt) const;

    // Snap calculation
    QPointF getSnappedPosition(const QPointF& rawPos);

signals:
    void mouseCoordinatesChanged(const QPointF& worldPos);
    void snapToggled(bool enabled);
    void gridSnapToggled(bool enabled);
    void orthoToggled(bool enabled);
    void polarToggled(bool enabled);
    void commandExecuted(const QString& cmd);
    void pageSetupRequested();
    void propertiesRequested();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void drawCrosshair(QPainter* painter);
    void drawViewportOverlays(QPainter* painter);
    void handleNavOverlayClick(const QPoint& pt);

    CadScene* m_scene{nullptr};
    Tool* m_currentTool{nullptr};
    SnapManager* m_snapManager{nullptr};
    bool m_orthoEnabled{false};

    // Crosshair & cursor state
    bool m_crosshairVisible{true};
    bool m_mouseInsideViewport{false};
    QPoint m_mouseScreenPos;

    // Panning state
    bool m_isPanning{false};
    QPoint m_panStartPoint;
    bool m_spacePressed{false};

    // Viewport tabs
    int m_activeSpaceTab{0}; // 0 = Model, 1 = Layout

    // Theme
    CadTheme::ThemeMode m_themeMode{CadTheme::ThemeMode::Dark};
};
