#include "commands/MirrorEntityCommand.h"
#include "geometry/CadEntity.h"

MirrorEntityCommand::MirrorEntityCommand(CadDocument* document,
                                         const std::vector<QUuid>& entityIds,
                                         const QPointF& p1,
                                         const QPointF& p2,
                                         bool makeCopy,
                                         QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_document(document)
    , m_entityIds(entityIds)
    , m_p1(p1)
    , m_p2(p2)
    , m_makeCopy(makeCopy)
{
    if (m_makeCopy) {
        setText(QStringLiteral("Mirror Copy %1 entities").arg(m_entityIds.size()));
    } else {
        setText(QStringLiteral("Mirror %1 entities").arg(m_entityIds.size()));
    }
}

void MirrorEntityCommand::redo() {
    if (!m_document) return;

    if (m_makeCopy) {
        if (m_firstRun) {
            m_createdCopies.clear();
            for (const auto& id : m_entityIds) {
                auto entity = m_document->findEntity(id);
                if (entity) {
                    auto clone = entity->clone();
                    clone->setId(QUuid::createUuid());
                    clone->mirror(m_p1, m_p2);
                    m_createdCopies.push_back(clone);
                }
            }
            m_firstRun = false;
        }

        for (const auto& copy : m_createdCopies) {
            m_document->addEntity(copy);
        }
    } else {
        for (const auto& id : m_entityIds) {
            auto entity = m_document->findEntity(id);
            if (entity) {
                entity->mirror(m_p1, m_p2);
                emit m_document->entityModified(entity);
            }
        }
    }

    emit m_document->documentChanged();
}

void MirrorEntityCommand::undo() {
    if (!m_document) return;

    if (m_makeCopy) {
        for (const auto& copy : m_createdCopies) {
            m_document->removeEntity(copy->id());
        }
    } else {
        for (const auto& id : m_entityIds) {
            auto entity = m_document->findEntity(id);
            if (entity) {
                entity->mirror(m_p1, m_p2);
                emit m_document->entityModified(entity);
            }
        }
    }

    emit m_document->documentChanged();
}
