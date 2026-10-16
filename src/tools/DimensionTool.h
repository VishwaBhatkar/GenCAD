#pragma once

#include "tools/Tool.h"
#include "geometry/DimensionEntity.h"
#include <QPointF>
#include <QUuid>

class CadCommandLine;

class DimensionTool : public Tool {
public:
    DimensionTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine = nullptr);

    void activate() override;
    void deactivate() override;

    void setDimensionType(DimensionType type) { m_dimType = type; }
    DimensionType dimensionType() const { return m_dimType; }

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    QString name() const override { return QStringLiteral("Dimension"); }

private:
    void cancel();

    enum class State {
        WaitingForP1,
        WaitingForP2,
        WaitingForDimPoint
    };

    State m_state{State::WaitingForP1};
    DimensionType m_dimType{DimensionType::Aligned};
    QPointF m_p1;
    QPointF m_p2;
    QUuid m_targetEntityId;
    int m_targetFeatureIndex{0};
    CadCommandLine* m_cmdLine{nullptr};
};
