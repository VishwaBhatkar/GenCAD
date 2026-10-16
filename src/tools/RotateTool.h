#pragma once

#include "tools/Tool.h"
#include <QPointF>
#include <vector>
#include <QUuid>

class RotateTool : public Tool {
public:
    RotateTool(CadDocument* doc, CadView* view);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    QString name() const override { return QStringLiteral("Rotate"); }

private:
    void cancel();

    enum class State {
        WaitingForBasePoint,
        WaitingForAngle
    };

    State m_state{State::WaitingForBasePoint};
    QPointF m_basePoint;
    std::vector<QUuid> m_targetIds;
};
