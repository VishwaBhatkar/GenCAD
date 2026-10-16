#pragma once

#include "commands/Command.h"
#include <vector>
#include <QPointF>

class RotateEntityCommand : public QUndoCommand {
public:
    RotateEntityCommand(CadDocument* document,
                        const std::vector<QUuid>& entityIds,
                        const QPointF& origin,
                        double angleRad,
                        QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CadDocument* m_document{nullptr};
    std::vector<QUuid> m_entityIds;
    QPointF m_origin;
    double m_angleRad{0.0};
};
