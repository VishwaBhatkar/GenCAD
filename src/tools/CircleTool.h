#pragma once

#include "tools/Tool.h"
#include <QPointF>

class CadCommandLine;

class CircleTool : public Tool {
public:
    CircleTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine = nullptr);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    void commitExact(double radius, double diameter = 0.0);

    QString name() const override { return QStringLiteral("Circle"); }

private:
    void cancel();

    enum class State {
        WaitingForCenter,
        WaitingForRadius
    };

    State m_state{State::WaitingForCenter};
    QPointF m_center;
    CadCommandLine* m_cmdLine{nullptr};
};
