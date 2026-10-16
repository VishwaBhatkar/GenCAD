#pragma once

#include "commands/Command.h"
#include <vector>
#include <memory>
#include <QPointF>

class CadEntity;

class MirrorEntityCommand : public QUndoCommand {
public:
    MirrorEntityCommand(CadDocument* document,
                        const std::vector<QUuid>& entityIds,
                        const QPointF& p1,
                        const QPointF& p2,
                        bool makeCopy = false,
                        QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CadDocument* m_document{nullptr};
    std::vector<QUuid> m_entityIds;
    QPointF m_p1;
    QPointF m_p2;
    bool m_makeCopy{false};
    std::vector<std::shared_ptr<CadEntity>> m_createdCopies;
    bool m_firstRun{true};
};
