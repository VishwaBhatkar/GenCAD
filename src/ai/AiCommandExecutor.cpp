#include "ai/AiCommandExecutor.h"
#include "ai/AiValidator.h"
#include "document/CadDocument.h"
#include "layers/LayerManager.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"
#include "geometry/ArcEntity.h"
#include "geometry/PolylineEntity.h"
#include "geometry/TextEntity.h"
#include "geometry/DimensionEntity.h"
#include "commands/CreateEntityCommand.h"
#include "commands/DeleteEntityCommand.h"
#include "commands/MoveEntityCommand.h"
#include "commands/RotateEntityCommand.h"
#include "commands/MirrorEntityCommand.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::shared_ptr<CadEntity> AiCommandExecutor::entityFromOperation(const AiOperation& op) {
    std::shared_ptr<CadEntity> entity;

    switch (op.type) {
        case AiOperationType::CreateLine:
            entity = std::make_shared<LineEntity>(op.p1, op.p2);
            break;

        case AiOperationType::CreateCircle:
            entity = std::make_shared<CircleEntity>(op.p1, op.radius);
            break;

        case AiOperationType::CreateRectangle:
            entity = std::make_shared<RectangleEntity>(op.p1, op.width, op.height);
            break;

        case AiOperationType::CreateArc:
            entity = std::make_shared<ArcEntity>(op.p1, op.radius, op.startAngle, op.sweepAngle);
            break;

        case AiOperationType::CreatePolyline:
            entity = std::make_shared<PolylineEntity>(op.points, op.closed);
            break;

        case AiOperationType::CreateText:
            entity = std::make_shared<TextEntity>(op.p1, op.text, op.height, op.rotation);
            break;

        case AiOperationType::CreateDimension:
            entity = std::make_shared<DimensionEntity>(op.p1, op.p2, op.p3);
            break;

        default:
            return nullptr;
    }

    if (entity) {
        if (!op.layer.isEmpty()) {
            entity->setLayer(op.layer);
        }
        if (op.hasCustomColor) {
            entity->setColor(op.color);
        }
    }

    return entity;
}

std::vector<std::shared_ptr<CadEntity>> AiCommandExecutor::instantiateEntities(const AiProposal& proposal) {
    std::vector<std::shared_ptr<CadEntity>> list;
    for (const auto& op : proposal.operations) {
        auto ent = entityFromOperation(op);
        if (ent) {
            list.push_back(ent);
        }
    }
    return list;
}

AiMacroCommand::AiMacroCommand(CadDocument* document, const AiProposal& proposal, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_document(document)
    , m_proposal(proposal)
{
    QString desc = proposal.description.isEmpty() ? QStringLiteral("AI Generated Changes") : proposal.description;
    setText(desc);
    buildCommands();
}

void AiMacroCommand::buildCommands() {
    if (!m_document) return;

    for (const auto& op : m_proposal.operations) {
        switch (op.type) {
            case AiOperationType::CreateLine:
            case AiOperationType::CreateCircle:
            case AiOperationType::CreateRectangle:
            case AiOperationType::CreateArc:
            case AiOperationType::CreatePolyline:
            case AiOperationType::CreateText:
            case AiOperationType::CreateDimension: {
                // Ensure layer exists
                if (!op.layer.isEmpty() && m_document->layerManager()) {
                    if (!m_document->layerManager()->findLayer(op.layer)) {
                        m_document->layerManager()->addLayer(op.layer);
                    }
                }
                auto entity = AiCommandExecutor::entityFromOperation(op);
                if (entity) {
                    m_createdEntities.push_back(entity);
                    m_subCommands.push_back(std::make_unique<CreateEntityCommand>(m_document, entity));
                }
                break;
            }

            case AiOperationType::DeleteEntity: {
                auto target = m_document->findEntity(op.targetId);
                if (target) {
                    m_subCommands.push_back(std::make_unique<DeleteEntityCommand>(m_document, target));
                }
                break;
            }

            case AiOperationType::ModifyMove: {
                auto target = m_document->findEntity(op.targetId);
                if (target) {
                    std::vector<QUuid> ids = { op.targetId };
                    m_subCommands.push_back(std::make_unique<MoveEntityCommand>(m_document, ids, op.delta.x(), op.delta.y()));
                }
                break;
            }

            case AiOperationType::ModifyRotate: {
                auto target = m_document->findEntity(op.targetId);
                if (target) {
                    std::vector<QUuid> ids = { op.targetId };
                    double rad = op.angleDeg * M_PI / 180.0;
                    m_subCommands.push_back(std::make_unique<RotateEntityCommand>(m_document, ids, op.origin, rad));
                }
                break;
            }

            case AiOperationType::ModifyMirror: {
                auto target = m_document->findEntity(op.targetId);
                if (target) {
                    std::vector<QUuid> ids = { op.targetId };
                    m_subCommands.push_back(std::make_unique<MirrorEntityCommand>(m_document, ids, op.p1, op.p2, false));
                }
                break;
            }

            case AiOperationType::AddLayer: {
                if (m_document->layerManager() && !op.layerName.isEmpty()) {
                    m_document->layerManager()->addLayer(op.layerName, op.layerColor);
                }
                break;
            }

            default:
                break;
        }
    }
}

void AiMacroCommand::redo() {
    for (auto& cmd : m_subCommands) {
        if (cmd) {
            cmd->redo();
        }
    }
}

void AiMacroCommand::undo() {
    // Undo in reverse order
    for (auto it = m_subCommands.rbegin(); it != m_subCommands.rend(); ++it) {
        if (*it) {
            (*it)->undo();
        }
    }
}

bool AiCommandExecutor::executeProposal(const AiProposal& proposal,
                                        CadDocument* doc,
                                        QString* errorMessage,
                                        AiMacroCommand** outCommand) {
    if (!doc) {
        if (errorMessage) *errorMessage = QStringLiteral("Document is null");
        return false;
    }

    AiValidationResult valRes = AiValidator::validateProposal(proposal, doc);
    if (!valRes.isValid) {
        if (errorMessage) *errorMessage = valRes.errorString();
        return false;
    }

    auto macroCmd = new AiMacroCommand(doc, proposal);
    if (outCommand) {
        *outCommand = macroCmd;
    }

    if (doc->undoStack()) {
        doc->undoStack()->push(macroCmd);
    } else {
        // Fallback if no undo stack: run redo directly and keep ownership
        macroCmd->redo();
        delete macroCmd;
    }

    return true;
}
