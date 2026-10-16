#pragma once

#include "tools/Tool.h"
#include <QPointF>

class CadCommandLine;

class EllipseTool : public Tool {
public:
    EllipseTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine = nullptr);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    void commitExact(double majorAxis, double minorAxis, double rotationDeg);

    QString name() const override { return QStringLiteral("Ellipse"); }

private:
    void cancel();

    enum class State {
        WaitingForCenter,
        WaitingForMajorAxis,
        WaitingForMinorAxis
    };

    State m_state{State::WaitingForCenter};
    QPointF m_center;
    double m_radiusX{100.0};
    double m_radiusY{50.0};
    double m_rotationDeg{0.0};
    CadCommandLine* m_cmdLine{nullptr};
};
