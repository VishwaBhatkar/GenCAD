#pragma once

#include "tools/Tool.h"
#include <QPointF>
#include <memory>

class CadEntity;

class OffsetTool : public Tool {
public:
    OffsetTool(CadDocument* doc, CadView* view);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    QString name() const override { return QStringLiteral("Offset"); }

private:
    void cancel();
    std::shared_ptr<CadEntity> createOffsetEntity(const CadEntity* entity, const QPointF& targetPt) const;

    enum class State {
        WaitingForEntity,
        WaitingForSidePoint
    };

    State m_state{State::WaitingForEntity};
    std::shared_ptr<CadEntity> m_selectedEntity;
};
