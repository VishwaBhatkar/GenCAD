#pragma once

#include "tools/Tool.h"

class ExtendTool : public Tool {
public:
    ExtendTool(CadDocument* doc, CadView* view);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    QString name() const override { return QStringLiteral("Extend"); }
};
