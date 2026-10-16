#pragma once

#include "tools/Tool.h"
#include <QPointF>
#include <vector>

class MoveTool : public Tool {
public:
    MoveTool(CadDocument* doc, CadView* view);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    QString name() const override { return QStringLiteral("Move"); }

private:
    void cancel();

    enum class State {
        WaitingForBasePoint,
        WaitingForDestinationPoint
    };

    State m_state{State::WaitingForBasePoint};
    QPointF m_basePoint;
    std::vector<QUuid> m_entitiesToMove;
};
