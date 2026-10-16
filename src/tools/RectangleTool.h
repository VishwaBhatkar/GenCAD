#pragma once

#include "tools/Tool.h"
#include <QPointF>

class CadCommandLine;

class RectangleTool : public Tool {
public:
    RectangleTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine = nullptr);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    void commitExact(double width, double height);

    QString name() const override { return QStringLiteral("Rectangle"); }

private:
    void cancel();

    enum class State {
        WaitingForFirstCorner,
        WaitingForSecondCorner
    };

    State m_state{State::WaitingForFirstCorner};
    QPointF m_p1;
    CadCommandLine* m_cmdLine{nullptr};
};
