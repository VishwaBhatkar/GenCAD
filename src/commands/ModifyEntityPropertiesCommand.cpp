#include "commands/ModifyEntityPropertiesCommand.h"
#include "document/CadDocument.h"

ModifyEntityPropertiesCommand::ModifyEntityPropertiesCommand(CadDocument* doc,
                                                             std::shared_ptr<CadEntity> targetEntity,
                                                             std::shared_ptr<CadEntity> oldState,
                                                             std::shared_ptr<CadEntity> newState,
                                                             const QString& description,
                                                             QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_document(doc)
    , m_entityId(targetEntity ? targetEntity->id() : QUuid())
    , m_oldState(oldState)
    , m_newState(newState)
{
    setText(description);
}

void ModifyEntityPropertiesCommand::applyState(const std::shared_ptr<CadEntity>& from, std::shared_ptr<CadEntity>& to) {
    if (!from || !to) return;
    QJsonObject json = from->toJson();
    to->fromJson(json);
}

void ModifyEntityPropertiesCommand::redo() {
    if (!m_document) return;
    auto entity = m_document->findEntity(m_entityId);
    if (entity && m_newState) {
        applyState(m_newState, entity);
        emit m_document->entityModified(entity);
        emit m_document->documentChanged();
    }
}

void ModifyEntityPropertiesCommand::undo() {
    if (!m_document) return;
    auto entity = m_document->findEntity(m_entityId);
    if (entity && m_oldState) {
        applyState(m_oldState, entity);
        emit m_document->entityModified(entity);
        emit m_document->documentChanged();
    }
}
