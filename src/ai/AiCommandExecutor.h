#pragma once

#include <QUndoCommand>
#include <QString>
#include <vector>
#include <memory>
#include "ai/AiDsl.h"
#include "geometry/CadEntity.h"

class CadDocument;

class AiMacroCommand : public QUndoCommand {
public:
    AiMacroCommand(CadDocument* document, const AiProposal& proposal, QUndoCommand* parent = nullptr);
    ~AiMacroCommand() override = default;

    void undo() override;
    void redo() override;

    const std::vector<std::shared_ptr<CadEntity>>& createdEntities() const { return m_createdEntities; }

private:
    void buildCommands();

    CadDocument* m_document{nullptr};
    AiProposal m_proposal;
    std::vector<std::shared_ptr<CadEntity>> m_createdEntities;
    std::vector<std::unique_ptr<QUndoCommand>> m_subCommands;
    bool m_firstRun{true};
};

class AiCommandExecutor {
public:
    static bool executeProposal(const AiProposal& proposal,
                               CadDocument* doc,
                               QString* errorMessage = nullptr,
                               AiMacroCommand** outCommand = nullptr);

    static std::vector<std::shared_ptr<CadEntity>> instantiateEntities(const AiProposal& proposal);
    static std::shared_ptr<CadEntity> entityFromOperation(const AiOperation& op);
};
