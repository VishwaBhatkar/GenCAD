#include "ai/AiFloorPlanSpec.h"
#include "geometry/LineEntity.h"
#include "geometry/ArcEntity.h"
#include "geometry/TextEntity.h"
#include "geometry/DimensionEntity.h"
#include <cmath>

AiProposal FloorPlanLayout::toAiProposal() const {
    AiProposal proposal;
    proposal.description = QStringLiteral("Architectural Floor Plan: %1 (%2m x %3m)")
                           .arg(config.name)
                           .arg(config.width / 1000.0, 0, 'f', 1)
                           .arg(config.depth / 1000.0, 0, 'f', 1);
    proposal.prompt = QStringLiteral("Generate architectural floor plan %1x%2m")
                      .arg(config.width / 1000.0, 0, 'f', 1)
                      .arg(config.depth / 1000.0, 0, 'f', 1);

    // 1. Setup AIA standard layers
    auto addLayerOp = [&](const QString& name, const QColor& color) {
        AiOperation op;
        op.type = AiOperationType::AddLayer;
        op.layerName = name;
        op.layerColor = color;
        proposal.operations.push_back(op);
    };

    addLayerOp(QStringLiteral("A-WALL"), QColor(230, 230, 230));
    addLayerOp(QStringLiteral("A-DOOR"), QColor(255, 167, 38));
    addLayerOp(QStringLiteral("A-GLAZ"), QColor(38, 198, 218));
    addLayerOp(QStringLiteral("A-FLOR-STRS"), QColor(176, 190, 197));
    addLayerOp(QStringLiteral("A-ANNO-TEXT"), QColor(255, 235, 59));
    addLayerOp(QStringLiteral("A-ANNO-DIMS"), QColor(66, 165, 245));

    // 2. Walls
    for (const auto& w : walls) {
        AiOperation op;
        op.type = AiOperationType::CreateLine;
        op.p1 = w.p1;
        op.p2 = w.p2;
        op.layer = w.layer;
        proposal.operations.push_back(op);
    }

    // 3. Doors (Door Leaf Line + 90° Swing Arc)
    for (const auto& d : doors) {
        // Door leaf
        AiOperation leaf;
        leaf.type = AiOperationType::CreateLine;
        leaf.p1 = d.hinge;
        leaf.p2 = d.strike;
        leaf.layer = d.layer;
        proposal.operations.push_back(leaf);

        // Door swing arc
        AiOperation arc;
        arc.type = AiOperationType::CreateArc;
        arc.p1 = d.swingCenter;
        arc.radius = d.swingRadius;
        arc.startAngle = d.startAngleDeg;
        arc.sweepAngle = d.sweepAngleDeg;
        arc.layer = d.layer;
        proposal.operations.push_back(arc);
    }

    // 4. Windows (Sill lines + Glazing)
    for (const auto& win : windows) {
        AiOperation line1;
        line1.type = AiOperationType::CreateLine;
        line1.p1 = win.p1;
        line1.p2 = win.p2;
        line1.layer = win.layer;
        proposal.operations.push_back(line1);
    }

    // 5. Stairs (Stringers, Treads, Direction arrow, "UP" Text)
    for (const auto& st : stairs) {
        double x1 = st.bounds.left();
        double y1 = st.bounds.top();
        double x2 = st.bounds.right();
        double y2 = st.bounds.bottom();

        // Left & right stringers
        AiOperation s1;
        s1.type = AiOperationType::CreateLine;
        s1.p1 = QPointF(x1, y1);
        s1.p2 = QPointF(x1, y2);
        s1.layer = st.layer;
        proposal.operations.push_back(s1);

        AiOperation s2;
        s2.type = AiOperationType::CreateLine;
        s2.p1 = QPointF(x2, y1);
        s2.p2 = QPointF(x2, y2);
        s2.layer = st.layer;
        proposal.operations.push_back(s2);

        // Treads
        int steps = std::max(2, st.stepCount);
        double stepDy = st.bounds.height() / steps;
        for (int i = 0; i <= steps; ++i) {
            double y = y1 + i * stepDy;
            AiOperation tr;
            tr.type = AiOperationType::CreateLine;
            tr.p1 = QPointF(x1, y);
            tr.p2 = QPointF(x2, y);
            tr.layer = st.layer;
            proposal.operations.push_back(tr);
        }

        // Direction arrow
        double midX = (x1 + x2) * 0.5;
        AiOperation arrow;
        arrow.type = AiOperationType::CreateLine;
        arrow.p1 = QPointF(midX, y1 + 150.0);
        arrow.p2 = QPointF(midX, y2 - 200.0);
        arrow.layer = st.layer;
        proposal.operations.push_back(arrow);

        // "UP" label
        AiOperation upTxt;
        upTxt.type = AiOperationType::CreateText;
        upTxt.p1 = QPointF(midX - 60.0, y1 + 100.0);
        upTxt.text = st.direction;
        upTxt.height = 100.0;
        upTxt.layer = st.layer;
        proposal.operations.push_back(upTxt);
    }

    // 6. Room Labels
    for (const auto& lbl : labels) {
        AiOperation txt;
        txt.type = AiOperationType::CreateText;
        txt.p1 = lbl.position;
        txt.text = lbl.text;
        txt.height = lbl.height;
        txt.layer = lbl.layer;
        proposal.operations.push_back(txt);
    }

    // 7. Dimensions
    for (const auto& dm : dimensions) {
        AiOperation dim;
        dim.type = AiOperationType::CreateDimension;
        dim.p1 = dm.p1;
        dim.p2 = dm.p2;
        dim.p3 = dm.dimPoint;
        dim.layer = dm.layer;
        proposal.operations.push_back(dim);
    }

    return proposal;
}

std::vector<std::shared_ptr<CadEntity>> FloorPlanLayout::instantiateEntities() const {
    std::vector<std::shared_ptr<CadEntity>> entities;

    // 1. Walls
    for (const auto& w : walls) {
        auto line = std::make_shared<LineEntity>(w.p1, w.p2);
        line->setLayer(w.layer);
        entities.push_back(line);
    }

    // 2. Doors
    for (const auto& d : doors) {
        auto leaf = std::make_shared<LineEntity>(d.hinge, d.strike);
        leaf->setLayer(d.layer);
        entities.push_back(leaf);

        auto arc = std::make_shared<ArcEntity>(d.swingCenter, d.swingRadius, d.startAngleDeg, d.sweepAngleDeg);
        arc->setLayer(d.layer);
        entities.push_back(arc);
    }

    // 3. Windows
    for (const auto& win : windows) {
        auto line = std::make_shared<LineEntity>(win.p1, win.p2);
        line->setLayer(win.layer);
        entities.push_back(line);
    }

    // 4. Stairs
    for (const auto& st : stairs) {
        double x1 = st.bounds.left();
        double y1 = st.bounds.top();
        double x2 = st.bounds.right();
        double y2 = st.bounds.bottom();

        auto s1 = std::make_shared<LineEntity>(QPointF(x1, y1), QPointF(x1, y2));
        s1->setLayer(st.layer);
        entities.push_back(s1);

        auto s2 = std::make_shared<LineEntity>(QPointF(x2, y1), QPointF(x2, y2));
        s2->setLayer(st.layer);
        entities.push_back(s2);

        int steps = std::max(2, st.stepCount);
        double stepDy = st.bounds.height() / steps;
        for (int i = 0; i <= steps; ++i) {
            double y = y1 + i * stepDy;
            auto tr = std::make_shared<LineEntity>(QPointF(x1, y), QPointF(x2, y));
            tr->setLayer(st.layer);
            entities.push_back(tr);
        }

        double midX = (x1 + x2) * 0.5;
        auto arrow = std::make_shared<LineEntity>(QPointF(midX, y1 + 150.0), QPointF(midX, y2 - 200.0));
        arrow->setLayer(st.layer);
        entities.push_back(arrow);

        auto upTxt = std::make_shared<TextEntity>(QPointF(midX - 60.0, y1 + 100.0), st.direction, 100.0);
        upTxt->setLayer(st.layer);
        entities.push_back(upTxt);
    }

    // 5. Room Labels
    for (const auto& lbl : labels) {
        auto txt = std::make_shared<TextEntity>(lbl.position, lbl.text, lbl.height);
        txt->setLayer(lbl.layer);
        entities.push_back(txt);
    }

    // 6. Dimensions
    for (const auto& dm : dimensions) {
        auto dim = std::make_shared<DimensionEntity>(dm.p1, dm.p2, dm.dimPoint, DimensionType::Aligned);
        dim->setLayer(dm.layer);
        entities.push_back(dim);
    }

    return entities;
}
