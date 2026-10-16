#pragma once

#include "commands/Command.h"
#include <vector>
#include <memory>

class CadEntity;

class CopyEntityCommand : public QUndoCommand {
public:
    CopyEntityCommand(CadDocument* document,
                      const std::vector<QUuid>& entityIds,
                      double deltaX,
                      double deltaY,
                      QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

    const std::vector<std::shared_ptr<CadEntity>>& createdCopies() const { return m_createdCopies; }

private:
    CadDocument* m_document{nullptr};
    std::vector<QUuid> m_entityIds;
    double m_deltaX{0.0};
    double m_deltaY{0.0};
    std::vector<std::shared_ptr<CadEntity>> m_createdCopies;
    bool m_firstRun{true};
};
