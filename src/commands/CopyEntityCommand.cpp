#include "commands/CopyEntityCommand.h"
#include "geometry/CadEntity.h"

CopyEntityCommand::CopyEntityCommand(CadDocument* document,
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
        setText(QStringLiteral("Copy entity"));
    } else {
        setText(QStringLiteral("Copy %1 entities").arg(m_entityIds.size()));
    }
}

void CopyEntityCommand::redo() {
    if (!m_document) return;

    if (m_firstRun) {
        m_createdCopies.clear();
        for (const auto& id : m_entityIds) {
            auto entity = m_document->findEntity(id);
            if (entity) {
                auto clone = entity->clone();
                clone->setId(QUuid::createUuid());
                clone->move(m_deltaX, m_deltaY);
                clone->setSelected(false);
                m_createdCopies.push_back(clone);
            }
        }
        m_firstRun = false;
    }

    for (const auto& copy : m_createdCopies) {
        m_document->addEntity(copy);
    }
    emit m_document->documentChanged();
}

void CopyEntityCommand::undo() {
    if (!m_document) return;

    for (const auto& copy : m_createdCopies) {
        m_document->removeEntity(copy->id());
    }
    emit m_document->documentChanged();
}
