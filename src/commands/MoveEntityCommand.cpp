#include "commands/MoveEntityCommand.h"

MoveEntityCommand::MoveEntityCommand(CadDocument* document,
                                     const std::vector<QUuid>& entityIds,
                                     double deltaX,
                                     double deltaY,
                                     QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_document(document)
    , m_entityIds(entityIds)
    , m_deltaX(deltaX)
    , m_deltaY(deltaY)
{
    if (m_entityIds.size() == 1) {
        setText(QStringLiteral("Move entity"));
    } else {
        setText(QStringLiteral("Move %1 entities").arg(m_entityIds.size()));
    }
}

void MoveEntityCommand::redo() {
    if (!m_document) return;
    for (const auto& id : m_entityIds) {
        auto entity = m_document->findEntity(id);
        if (entity) {
            entity->move(m_deltaX, m_deltaY);
            emit m_document->entityModified(entity);
        }
    }
    emit m_document->documentChanged();
}

void MoveEntityCommand::undo() {
    if (!m_document) return;
    for (const auto& id : m_entityIds) {
        auto entity = m_document->findEntity(id);
        if (entity) {
            entity->move(-m_deltaX, -m_deltaY);
            emit m_document->entityModified(entity);
        }
    }
    emit m_document->documentChanged();
}
