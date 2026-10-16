#pragma once

#include "tools/Tool.h"
#include <QPointF>
#include <QRectF>

class SelectTool : public Tool {
public:
    SelectTool(CadDocument* doc, CadView* view);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseMove(QMouseEvent* event, const QPointF& worldPos) override;
    void mouseRelease(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    QString name() const override { return QStringLiteral("Select"); }

private:
    bool m_isBoxSelecting{false};
    QPointF m_boxStart;
    QPointF m_boxEnd;
};
