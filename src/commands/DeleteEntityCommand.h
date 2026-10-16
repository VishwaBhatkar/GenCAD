#pragma once

#include "commands/Command.h"
#include <vector>

class DeleteEntityCommand : public QUndoCommand {
public:
    DeleteEntityCommand(CadDocument* document, const std::vector<std::shared_ptr<CadEntity>>& entities, QUndoCommand* parent = nullptr);
    DeleteEntityCommand(CadDocument* document, std::shared_ptr<CadEntity> entity, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CadDocument* m_document{nullptr};
    std::vector<std::shared_ptr<CadEntity>> m_entities;
};
