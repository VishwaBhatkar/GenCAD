#pragma once

#include "tools/Tool.h"
#include <QPointF>
#include <vector>
#include <QUuid>

class CopyTool : public Tool {
public:
    CopyTool(CadDocument* doc, CadView* view);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    QString name() const override { return QStringLiteral("Copy"); }

private:
    void cancel();

    enum class State {
        WaitingForBasePoint,
        WaitingForDestPoint
    };

    State m_state{State::WaitingForBasePoint};
    QPointF m_basePoint;
    std::vector<QUuid> m_targetIds;
};
