#pragma once

#include "tools/Tool.h"
#include <QPointF>
#include <vector>
#include <QUuid>

class MirrorTool : public Tool {
public:
    MirrorTool(CadDocument* doc, CadView* view);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    QString name() const override { return QStringLiteral("Mirror"); }

private:
    void cancel();

    enum class State {
        WaitingForAxisP1,
        WaitingForAxisP2
    };

    State m_state{State::WaitingForAxisP1};
    QPointF m_p1;
    std::vector<QUuid> m_targetIds;
};
