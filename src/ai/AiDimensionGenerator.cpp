#include "ai/AiDimensionGenerator.h"
#include "document/CadDocument.h"
#include "layers/LayerManager.h"
#include "geometry/RectangleEntity.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include <cmath>

AiProposal AiDimensionGenerator::generateDimensions(const CadDocument* doc,
                                                   const std::vector<QUuid>& targetIds,
                                                   double offset) {
    AiProposal proposal;
    proposal.description = QStringLiteral("AI Generated Dimensions");
    proposal.prompt = QStringLiteral("Add dimensions to geometry");

    if (!doc) return proposal;

    // Check if Dimensions layer should be added
    bool hasDimLayer = false;
    if (doc->layerManager()) {
        hasDimLayer = (doc->layerManager()->findLayer(QStringLiteral("Dimensions")) != nullptr);
    }
    if (!hasDimLayer) {
        AiOperation addLayerOp;
        addLayerOp.type = AiOperationType::AddLayer;
        addLayerOp.layerName = QStringLiteral("Dimensions");
        addLayerOp.layerColor = QColor(0, 180, 255);
        proposal.operations.push_back(addLayerOp);
    }

    std::vector<std::shared_ptr<CadEntity>> entitiesToDimension;
    if (!targetIds.empty()) {
        for (const auto& id : targetIds) {
            auto ent = doc->findEntity(id);
            if (ent) entitiesToDimension.push_back(ent);
        }
    } else {
        for (const auto& ent : doc->entities()) {
            if (ent && ent->type() != EntityType::Dimension) {
                entitiesToDimension.push_back(ent);
            }
        }
    }

    if (entitiesToDimension.empty()) {
        proposal.description = QStringLiteral("No geometry available to dimension");
        return proposal;
    }

    bool addedSpecific = false;

    for (const auto& ent : entitiesToDimension) {
        if (ent->type() == EntityType::Rectangle) {
            auto rect = std::dynamic_pointer_cast<RectangleEntity>(ent);
            if (rect) {
                const auto& corners = rect->corners();
                // corners: 0=TL, 1=TR, 2=BR, 3=BL
                QPointF tl = corners[0];
                QPointF tr = corners[1];
                QPointF br = corners[2];

                // Top horizontal dimension
                AiOperation topDim;
                topDim.type = AiOperationType::CreateDimension;
                topDim.p1 = tl;
                topDim.p2 = tr;
                topDim.p3 = QPointF((tl.x() + tr.x()) / 2.0, tl.y() - offset);
                topDim.layer = QStringLiteral("Dimensions");
                proposal.operations.push_back(topDim);

                // Right vertical dimension
                AiOperation rightDim;
                rightDim.type = AiOperationType::CreateDimension;
                rightDim.p1 = tr;
                rightDim.p2 = br;
                rightDim.p3 = QPointF(tr.x() + offset, (tr.y() + br.y()) / 2.0);
                rightDim.layer = QStringLiteral("Dimensions");
                proposal.operations.push_back(rightDim);

                addedSpecific = true;
            }
        } else if (ent->type() == EntityType::Line) {
            auto line = std::dynamic_pointer_cast<LineEntity>(ent);
            if (line) {
                QPointF p1 = line->startPoint();
                QPointF p2 = line->endPoint();
                double dx = p2.x() - p1.x();
                double dy = p2.y() - p1.y();
                double len = std::hypot(dx, dy);
                if (len > 1e-4) {
                    double nx = -dy / len;
                    double ny = dx / len;
                    QPointF mid((p1.x() + p2.x()) / 2.0 + nx * offset,
                                (p1.y() + p2.y()) / 2.0 + ny * offset);

                    AiOperation lineDim;
                    lineDim.type = AiOperationType::CreateDimension;
                    lineDim.p1 = p1;
                    lineDim.p2 = p2;
                    lineDim.p3 = mid;
                    lineDim.layer = QStringLiteral("Dimensions");
                    proposal.operations.push_back(lineDim);
                    addedSpecific = true;
                }
            }
        }
    }

    // If nothing specific was dimensioned or multiple entities exist, add overall bounding box dimensions
    if (!addedSpecific || targetIds.empty()) {
        QRectF bbox = doc->boundingBox();
        if (bbox.isValid() && bbox.width() > 1e-4 && bbox.height() > 1e-4) {
            // Overall width (bottom)
            AiOperation wDim;
            wDim.type = AiOperationType::CreateDimension;
            wDim.p1 = QPointF(bbox.left(), bbox.bottom());
            wDim.p2 = QPointF(bbox.right(), bbox.bottom());
            wDim.p3 = QPointF(bbox.center().x(), bbox.bottom() + offset * 1.5);
            wDim.layer = QStringLiteral("Dimensions");
            proposal.operations.push_back(wDim);

            // Overall height (left)
            AiOperation hDim;
            hDim.type = AiOperationType::CreateDimension;
            hDim.p1 = QPointF(bbox.left(), bbox.bottom());
            hDim.p2 = QPointF(bbox.left(), bbox.top());
            hDim.p3 = QPointF(bbox.left() - offset * 1.5, bbox.center().y());
            hDim.layer = QStringLiteral("Dimensions");
            proposal.operations.push_back(hDim);
        }
    }

    return proposal;
}
