#pragma once

#include <QUndoCommand>
#include <QUuid>
#include <memory>
#include "geometry/CadEntity.h"

class CadDocument;

class ModifyEntityPropertiesCommand : public QUndoCommand {
public:
    ModifyEntityPropertiesCommand(CadDocument* doc,
                                  std::shared_ptr<CadEntity> targetEntity,
                                  std::shared_ptr<CadEntity> oldState,
                                  std::shared_ptr<CadEntity> newState,
                                  const QString& description = QStringLiteral("Modify properties"),
                                  QUndoCommand* parent = nullptr);

    void redo() override;
    void undo() override;

private:
    void applyState(const std::shared_ptr<CadEntity>& from, std::shared_ptr<CadEntity>& to);

    CadDocument* m_document{nullptr};
    QUuid m_entityId;
    std::shared_ptr<CadEntity> m_oldState;
    std::shared_ptr<CadEntity> m_newState;
};
