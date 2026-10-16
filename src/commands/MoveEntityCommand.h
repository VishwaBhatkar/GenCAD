#pragma once

#include "commands/Command.h"
#include <vector>

class MoveEntityCommand : public QUndoCommand {
public:
    MoveEntityCommand(CadDocument* document,
                      const std::vector<QUuid>& entityIds,
                      double deltaX,
                      double deltaY,
                      QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CadDocument* m_document{nullptr};
    std::vector<QUuid> m_entityIds;
    double m_deltaX{0.0};
    double m_deltaY{0.0};
};
