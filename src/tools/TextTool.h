#pragma once

#include "tools/Tool.h"
#include <QPointF>

class TextTool : public Tool {
public:
    TextTool(CadDocument* doc, CadView* view);

    void activate() override;
    void deactivate() override;

    void mousePress(QMouseEvent* event, const QPointF& worldPos) override;
    void keyPress(QKeyEvent* event) override;

    QString name() const override { return QStringLiteral("Text"); }
};
