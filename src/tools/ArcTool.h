#pragma once

#include "tools/Tool.h"
#include <QPointF>

class CadCommandLine;

class ArcTool : public Tool {
public:
    ArcTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine = nullptr);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    void commitExact(double radius, double startAngle, double endAngle);

    QString name() const override { return QStringLiteral("Arc"); }

private:
    void cancel();

    enum class State {
        WaitingForCenter,
        WaitingForStartPoint,
        WaitingForEndPoint
    };

    State m_state{State::WaitingForCenter};
    QPointF m_center;
    double m_radius{0.0};
    double m_startAngle{0.0};
    CadCommandLine* m_cmdLine{nullptr};
};
