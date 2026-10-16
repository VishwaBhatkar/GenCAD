#include "commands/CreateEntityCommand.h"

CreateEntityCommand::CreateEntityCommand(CadDocument* document, std::shared_ptr<CadEntity> entity, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_document(document)
    , m_entity(entity)
{
    if (m_entity) {
        m_entityId = m_entity->id();
        setText(QStringLiteral("Create %1").arg(CadEntity::entityTypeToString(m_entity->type())));
    }
}

void CreateEntityCommand::redo() {
    if (!m_document || !m_entity) return;
    m_document->addEntity(m_entity);
}

void CreateEntityCommand::undo() {
    if (!m_document) return;
    m_document->removeEntity(m_entityId);
}
