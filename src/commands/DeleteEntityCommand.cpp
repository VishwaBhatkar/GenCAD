#include "commands/DeleteEntityCommand.h"

DeleteEntityCommand::DeleteEntityCommand(CadDocument* document, const std::vector<std::shared_ptr<CadEntity>>& entities, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_document(document)
    , m_entities(entities)
{
    if (m_entities.size() == 1) {
        setText(QStringLiteral("Delete %1").arg(CadEntity::entityTypeToString(m_entities.front()->type())));
    } else {
        setText(QStringLiteral("Delete %1 entities").arg(m_entities.size()));
    }
}

DeleteEntityCommand::DeleteEntityCommand(CadDocument* document, std::shared_ptr<CadEntity> entity, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_document(document)
{
    if (entity) {
        m_entities.push_back(entity);
        setText(QStringLiteral("Delete %1").arg(CadEntity::entityTypeToString(entity->type())));
    }
}

void DeleteEntityCommand::redo() {
    if (!m_document) return;
    for (const auto& entity : m_entities) {
        if (entity) {
            m_document->removeEntity(entity->id());
        }
    }
}

void DeleteEntityCommand::undo() {
    if (!m_document) return;
    for (const auto& entity : m_entities) {
        if (entity) {
            m_document->addEntity(entity);
        }
    }
}
