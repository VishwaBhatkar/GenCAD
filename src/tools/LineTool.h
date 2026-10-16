#pragma once

#include "tools/Tool.h"
#include <QPointF>

class CadCommandLine;

class LineTool : public Tool {
public:
    LineTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine = nullptr);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    void commitExact(double length, double angleDeg);
    void commitExactCoords(double x, double y);

    QString name() const override { return QStringLiteral("Line"); }
    bool isDirectionalOrthoConstrained() const override { return m_state == State::WaitingForSecondPoint; }

private:
    void cancel();

    enum class State {
        WaitingForFirstPoint,
        WaitingForSecondPoint
    };

    State m_state{State::WaitingForFirstPoint};
    QPointF m_p1;
    CadCommandLine* m_cmdLine{nullptr};
};
