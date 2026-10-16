#include "ai/AiValidator.h"
#include "document/CadDocument.h"
#include "layers/LayerManager.h"
#include <cmath>
#include <set>

bool AiValidator::isPointValid(const QPointF& pt) {
    return std::isfinite(pt.x()) && std::isfinite(pt.y());
}

double AiValidator::distance(const QPointF& p1, const QPointF& p2) {
    double dx = p1.x() - p2.x();
    double dy = p1.y() - p2.y();
    return std::sqrt(dx * dx + dy * dy);
}

AiValidationResult AiValidator::validateOperation(const AiOperation& op, const CadDocument* doc) {
    AiValidationResult res;

    switch (op.type) {
        case AiOperationType::CreateLine: {
            if (!isPointValid(op.p1) || !isPointValid(op.p2)) {
                res.errors.append(QStringLiteral("Line coordinates must be finite valid numbers"));
            } else if (distance(op.p1, op.p2) < 1e-6) {
                res.errors.append(QStringLiteral("Line start and end points must not be identical"));
            }
            break;
        }

        case AiOperationType::CreateCircle: {
            if (!isPointValid(op.p1)) {
                res.errors.append(QStringLiteral("Circle center must be finite valid numbers"));
            }
            if (op.radius <= 1e-6 || !std::isfinite(op.radius)) {
                res.errors.append(QStringLiteral("Circle radius must be strictly positive"));
            }
            break;
        }

        case AiOperationType::CreateRectangle: {
            if (!isPointValid(op.p1)) {
                res.errors.append(QStringLiteral("Rectangle corner must be finite valid numbers"));
            }
            if (op.width <= 1e-6 || !std::isfinite(op.width)) {
                res.errors.append(QStringLiteral("Rectangle width must be strictly positive"));
            }
            if (op.height <= 1e-6 || !std::isfinite(op.height)) {
                res.errors.append(QStringLiteral("Rectangle height must be strictly positive"));
            }
            break;
        }

        case AiOperationType::CreateArc: {
            if (!isPointValid(op.p1)) {
                res.errors.append(QStringLiteral("Arc center must be finite valid numbers"));
            }
            if (op.radius <= 1e-6 || !std::isfinite(op.radius)) {
                res.errors.append(QStringLiteral("Arc radius must be strictly positive"));
            }
            if (std::abs(op.sweepAngle) < 1e-6 || !std::isfinite(op.sweepAngle)) {
                res.errors.append(QStringLiteral("Arc sweep angle cannot be zero"));
            }
            break;
        }

        case AiOperationType::CreatePolyline: {
            if (op.points.size() < 2) {
                res.errors.append(QStringLiteral("Polyline must have at least 2 vertices"));
            }
            for (size_t i = 0; i < op.points.size(); ++i) {
                if (!isPointValid(op.points[i])) {
                    res.errors.append(QStringLiteral("Polyline vertex #%1 is invalid").arg(i + 1));
                    break;
                }
            }
            break;
        }

        case AiOperationType::CreateText: {
            if (!isPointValid(op.p1)) {
                res.errors.append(QStringLiteral("Text position must be finite valid numbers"));
            }
            if (op.text.trimmed().isEmpty()) {
                res.errors.append(QStringLiteral("Text content cannot be empty"));
            }
            if (op.height <= 1e-6 || !std::isfinite(op.height)) {
                res.errors.append(QStringLiteral("Text height must be strictly positive"));
            }
            break;
        }

        case AiOperationType::CreateDimension: {
            if (!isPointValid(op.p1) || !isPointValid(op.p2) || !isPointValid(op.p3)) {
                res.errors.append(QStringLiteral("Dimension points must be finite valid numbers"));
            } else if (distance(op.p1, op.p2) < 1e-6) {
                res.errors.append(QStringLiteral("Dimension start and end points must not be identical"));
            }
            break;
        }

        case AiOperationType::ModifyMove: {
            if (op.targetId.isNull()) {
                res.errors.append(QStringLiteral("Move operation requires a valid target entity UUID"));
            } else if (doc && !doc->findEntity(op.targetId)) {
                res.errors.append(QStringLiteral("Target entity %1 for Move does not exist in document")
                                  .arg(op.targetId.toString(QUuid::WithoutBraces)));
            }
            if (!isPointValid(op.delta)) {
                res.errors.append(QStringLiteral("Move delta contains invalid coordinates"));
            }
            break;
        }

        case AiOperationType::ModifyRotate: {
            if (op.targetId.isNull()) {
                res.errors.append(QStringLiteral("Rotate operation requires a valid target entity UUID"));
            } else if (doc && !doc->findEntity(op.targetId)) {
                res.errors.append(QStringLiteral("Target entity %1 for Rotate does not exist in document")
                                  .arg(op.targetId.toString(QUuid::WithoutBraces)));
            }
            if (!isPointValid(op.origin) || !std::isfinite(op.angleDeg)) {
                res.errors.append(QStringLiteral("Rotate origin or angle is invalid"));
            }
            break;
        }

        case AiOperationType::ModifyMirror: {
            if (op.targetId.isNull()) {
                res.errors.append(QStringLiteral("Mirror operation requires a valid target entity UUID"));
            } else if (doc && !doc->findEntity(op.targetId)) {
                res.errors.append(QStringLiteral("Target entity %1 for Mirror does not exist in document")
                                  .arg(op.targetId.toString(QUuid::WithoutBraces)));
            }
            if (!isPointValid(op.p1) || !isPointValid(op.p2)) {
                res.errors.append(QStringLiteral("Mirror axis points must be finite valid numbers"));
            } else if (distance(op.p1, op.p2) < 1e-6) {
                res.errors.append(QStringLiteral("Mirror axis points must not be identical"));
            }
            break;
        }

        case AiOperationType::DeleteEntity: {
            if (op.targetId.isNull()) {
                res.errors.append(QStringLiteral("Delete operation requires a valid target entity UUID"));
            } else if (doc && !doc->findEntity(op.targetId)) {
                res.errors.append(QStringLiteral("Target entity %1 for Delete does not exist in document")
                                  .arg(op.targetId.toString(QUuid::WithoutBraces)));
            }
            break;
        }

        case AiOperationType::AddLayer: {
            if (op.layerName.trimmed().isEmpty()) {
                res.errors.append(QStringLiteral("Layer name cannot be empty"));
            }
            break;
        }

        case AiOperationType::SetEntityLayer: {
            if (op.targetId.isNull()) {
                res.errors.append(QStringLiteral("SetEntityLayer requires a valid target entity UUID"));
            } else if (doc && !doc->findEntity(op.targetId)) {
                res.errors.append(QStringLiteral("Target entity %1 for SetEntityLayer does not exist in document")
                                  .arg(op.targetId.toString(QUuid::WithoutBraces)));
            }
            if (op.layer.trimmed().isEmpty()) {
                res.errors.append(QStringLiteral("Layer name cannot be empty"));
            }
            break;
        }

        default:
            res.errors.append(QStringLiteral("Unsupported or unknown operation type"));
            break;
    }

    res.isValid = res.errors.isEmpty();
    return res;
}

AiValidationResult AiValidator::validateProposal(const AiProposal& proposal, const CadDocument* doc) {
    AiValidationResult result;

    if (proposal.operations.empty()) {
        result.errors.append(QStringLiteral("Proposal contains no operations"));
        result.isValid = false;
        return result;
    }

    std::set<QString> knownLayers;
    if (doc && doc->layerManager()) {
        QStringList layers = doc->layerManager()->layerNames();
        for (const auto& l : layers) {
            knownLayers.insert(l);
        }
    }

    // First track layers created in this proposal
    for (const auto& op : proposal.operations) {
        if (op.type == AiOperationType::AddLayer && !op.layerName.isEmpty()) {
            knownLayers.insert(op.layerName);
        }
    }

    int opIndex = 1;
    for (const auto& op : proposal.operations) {
        AiValidationResult opRes = validateOperation(op, doc);
        if (!opRes.isValid) {
            for (const auto& err : opRes.errors) {
                result.errors.append(QStringLiteral("Op #%1 (%2): %3")
                                     .arg(opIndex)
                                     .arg(AiOperation::operationTypeToString(op.type))
                                     .arg(err));
            }
        }
        for (const auto& warn : opRes.warnings) {
            result.warnings.append(QStringLiteral("Op #%1: %2").arg(opIndex).arg(warn));
        }

        // Layer warning
        if (!op.layer.isEmpty() && knownLayers.find(op.layer) == knownLayers.end()) {
            result.warnings.append(QStringLiteral("Op #%1 references layer '%2' which does not currently exist")
                                   .arg(opIndex).arg(op.layer));
        }

        opIndex++;
    }

    result.isValid = result.errors.isEmpty();
    return result;
}
