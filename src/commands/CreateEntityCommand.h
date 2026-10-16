#pragma once

#include "commands/Command.h"

class CreateEntityCommand : public QUndoCommand {
public:
    CreateEntityCommand(CadDocument* document, std::shared_ptr<CadEntity> entity, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CadDocument* m_document{nullptr};
    std::shared_ptr<CadEntity> m_entity;
    QUuid m_entityId;
};
