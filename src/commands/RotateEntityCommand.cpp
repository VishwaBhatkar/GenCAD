#include "commands/RotateEntityCommand.h"

RotateEntityCommand::RotateEntityCommand(CadDocument* document,
                                         const std::vector<QUuid>& entityIds,
                                         const QPointF& origin,
                                         double angleRad,
                                         QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_document(document)
    , m_entityIds(entityIds)
    , m_origin(origin)
    , m_angleRad(angleRad)
{
    if (m_entityIds.size() == 1) {
        setText(QStringLiteral("Rotate entity"));
    } else {
        setText(QStringLiteral("Rotate %1 entities").arg(m_entityIds.size()));
    }
}

void RotateEntityCommand::redo() {
    if (!m_document) return;
    for (const auto& id : m_entityIds) {
        auto entity = m_document->findEntity(id);
        if (entity) {
            entity->rotate(m_origin, m_angleRad);
            emit m_document->entityModified(entity);
        }
    }
    emit m_document->documentChanged();
}

void RotateEntityCommand::undo() {
    if (!m_document) return;
    for (const auto& id : m_entityIds) {
        auto entity = m_document->findEntity(id);
        if (entity) {
            entity->rotate(m_origin, -m_angleRad);
            emit m_document->entityModified(entity);
        }
    }
    emit m_document->documentChanged();
}
