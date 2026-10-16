#pragma once

#include "tools/Tool.h"
#include <QPointF>
#include <vector>

class CadCommandLine;

class PolylineTool : public Tool {
public:
    PolylineTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine = nullptr);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    void commitExactSegment(double length, double angleDeg);
    void commitExact(double length, double angleDeg) { commitExactSegment(length, angleDeg); }
    void finish(bool close);

    QString name() const override { return QStringLiteral("Polyline"); }
    bool isDirectionalOrthoConstrained() const override { return !m_points.empty(); }

private:
    void cancel();

    std::vector<QPointF> m_points;
    CadCommandLine* m_cmdLine{nullptr};
};
