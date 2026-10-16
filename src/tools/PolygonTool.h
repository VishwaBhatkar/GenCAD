#pragma once

#include "tools/Tool.h"
#include <QPointF>

class CadCommandLine;

class PolygonTool : public Tool {
public:
    PolygonTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine = nullptr);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    void commitExact(double sides, double radius, double rotationDeg);

    QString name() const override { return QStringLiteral("Polygon"); }

private:
    void cancel();

    enum class State {
        WaitingForCenter,
        WaitingForRadius
    };

    State m_state{State::WaitingForCenter};
    QPointF m_center;
    int m_sides{6};
    double m_radius{100.0};
    double m_rotationDeg{0.0};
    CadCommandLine* m_cmdLine{nullptr};
};
