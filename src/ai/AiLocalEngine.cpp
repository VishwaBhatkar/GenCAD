#include "ai/AiLocalEngine.h"
#include "ai/AiDimensionGenerator.h"
#include "ai/AiFloorPlanProvider.h"
#include "document/CadDocument.h"
#include <QRegularExpression>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AiLocalEngine::AiLocalEngine() {
}

AiAnalysisReport AiLocalEngine::analyzeDocument(const CadDocument* doc) {
    return AiGeometryAnalyzer::analyze(doc);
}

AiProposal AiLocalEngine::cleanupDocument(const CadDocument* doc) {
    return AiGeometryAnalyzer::generateCleanupProposal(doc);
}

AiProposal AiLocalEngine::dimensionDocument(const CadDocument* doc, const std::vector<QUuid>& targetIds) {
    return AiDimensionGenerator::generateDimensions(doc, targetIds);
}

AiProposal AiLocalEngine::generateRoom(double width, double height, double wallThickness, const QPointF& origin) {
    AiProposal proposal;
    proposal.description = QStringLiteral("Architectural Room (%1m x %2m)")
                           .arg(width / 1000.0, 0, 'f', 1)
                           .arg(height / 1000.0, 0, 'f', 1);
    proposal.prompt = QStringLiteral("Create a %1m x %2m room").arg(width / 1000.0).arg(height / 1000.0);

    // Add layers
    AiOperation wallLayer;
    wallLayer.type = AiOperationType::AddLayer;
    wallLayer.layerName = QStringLiteral("A-WALL");
    wallLayer.layerColor = QColor(240, 240, 240);
    proposal.operations.push_back(wallLayer);

    AiOperation textLayer;
    textLayer.type = AiOperationType::AddLayer;
    textLayer.layerName = QStringLiteral("A-ANNO-TEXT");
    textLayer.layerColor = QColor(255, 215, 0);
    proposal.operations.push_back(textLayer);

    AiOperation dimLayer;
    dimLayer.type = AiOperationType::AddLayer;
    dimLayer.layerName = QStringLiteral("A-ANNO-DIMS");
    dimLayer.layerColor = QColor(0, 180, 255);
    proposal.operations.push_back(dimLayer);

    // 1. Outer Wall Rectangle
    AiOperation outerWall;
    outerWall.type = AiOperationType::CreateRectangle;
    outerWall.p1 = origin;
    outerWall.width = width;
    outerWall.height = height;
    outerWall.layer = QStringLiteral("A-WALL");
    proposal.operations.push_back(outerWall);

    // 2. Inner Wall Rectangle (if wall thickness is valid)
    if (wallThickness > 0.0 && width > 2 * wallThickness && height > 2 * wallThickness) {
        AiOperation innerWall;
        innerWall.type = AiOperationType::CreateRectangle;
        innerWall.p1 = QPointF(origin.x() + wallThickness, origin.y() + wallThickness);
        innerWall.width = width - 2 * wallThickness;
        innerWall.height = height - 2 * wallThickness;
        innerWall.layer = QStringLiteral("A-WALL");
        proposal.operations.push_back(innerWall);
    }

    // 3. Room Label Text
    AiOperation label;
    label.type = AiOperationType::CreateText;
    label.p1 = QPointF(origin.x() + width / 2.0 - 400.0, origin.y() + height / 2.0);
    label.text = QStringLiteral("ROOM %1m x %2m\nAREA: %3 m²")
                 .arg(width / 1000.0, 0, 'f', 1)
                 .arg(height / 1000.0, 0, 'f', 1)
                 .arg((width * height) / 1000000.0, 0, 'f', 1);
    label.height = 150.0;
    label.layer = QStringLiteral("A-ANNO-TEXT");
    proposal.operations.push_back(label);

    // 4. Outer Dimensions
    AiOperation dimW;
    dimW.type = AiOperationType::CreateDimension;
    dimW.p1 = origin;
    dimW.p2 = QPointF(origin.x() + width, origin.y());
    dimW.p3 = QPointF(origin.x() + width / 2.0, origin.y() - 300.0);
    dimW.layer = QStringLiteral("A-ANNO-DIMS");
    proposal.operations.push_back(dimW);

    AiOperation dimH;
    dimH.type = AiOperationType::CreateDimension;
    dimH.p1 = origin;
    dimH.p2 = QPointF(origin.x(), origin.y() + height);
    dimH.p3 = QPointF(origin.x() - 300.0, origin.y() + height / 2.0);
    dimH.layer = QStringLiteral("A-ANNO-DIMS");
    proposal.operations.push_back(dimH);

    return proposal;
}

AiProposal AiLocalEngine::generateDoor(double width, const QPointF& position, const QString& wall) {
    Q_UNUSED(wall);
    AiProposal proposal;
    proposal.description = QStringLiteral("Architectural Door (%1 mm swing)").arg(width);
    proposal.prompt = QStringLiteral("Add a %1mm door").arg(width);

    AiOperation doorLayer;
    doorLayer.type = AiOperationType::AddLayer;
    doorLayer.layerName = QStringLiteral("A-DOOR");
    doorLayer.layerColor = QColor(255, 179, 0);
    proposal.operations.push_back(doorLayer);

    // Door Leaf Line (open vertically)
    AiOperation leaf;
    leaf.type = AiOperationType::CreateLine;
    leaf.p1 = position;
    leaf.p2 = QPointF(position.x(), position.y() + width);
    leaf.layer = QStringLiteral("A-DOOR");
    proposal.operations.push_back(leaf);

    // Swing Arc (from (pos.x + width, pos.y) to (pos.x, pos.y + width))
    AiOperation swing;
    swing.type = AiOperationType::CreateArc;
    swing.p1 = position; // center
    swing.radius = width;
    swing.startAngle = 0.0;
    swing.sweepAngle = 90.0;
    swing.layer = QStringLiteral("A-DOOR");
    proposal.operations.push_back(swing);

    return proposal;
}

AiProposal AiLocalEngine::generateWindows(int count, double width, const CadDocument* doc) {
    AiProposal proposal;
    proposal.description = QStringLiteral("Architectural Windows (%1 openings, %2 mm wide)").arg(count).arg(width);

    AiOperation winLayer;
    winLayer.type = AiOperationType::AddLayer;
    winLayer.layerName = QStringLiteral("A-GLAZ");
    winLayer.layerColor = QColor(0, 229, 255);
    proposal.operations.push_back(winLayer);

    QRectF bounds = doc ? doc->boundingBox() : QRectF(0, 0, 5000, 4000);
    if (!bounds.isValid() || bounds.width() < 100) {
        bounds = QRectF(0, 0, 5000, 4000);
    }

    double spacing = bounds.width() / (count + 1);
    for (int i = 1; i <= count; ++i) {
        double cx = bounds.left() + i * spacing;
        double cy = bounds.top();

        // Window exterior line
        AiOperation l1;
        l1.type = AiOperationType::CreateLine;
        l1.p1 = QPointF(cx - width / 2.0, cy);
        l1.p2 = QPointF(cx + width / 2.0, cy);
        l1.layer = QStringLiteral("A-GLAZ");
        proposal.operations.push_back(l1);

        // Window interior/glazing line
        AiOperation l2;
        l2.type = AiOperationType::CreateLine;
        l2.p1 = QPointF(cx - width / 2.0, cy + 100.0);
        l2.p2 = QPointF(cx + width / 2.0, cy + 100.0);
        l2.layer = QStringLiteral("A-GLAZ");
        proposal.operations.push_back(l2);
    }

    return proposal;
}

AiProposal AiLocalEngine::generateStairs(int stepCount, double treadDepth, double stairWidth, const QPointF& startPos) {
    AiProposal proposal;
    proposal.description = QStringLiteral("Staircase Flight (%1 steps, %2 mm width)").arg(stepCount).arg(stairWidth);

    AiOperation stairLayer;
    stairLayer.type = AiOperationType::AddLayer;
    stairLayer.layerName = QStringLiteral("A-FLOR");
    stairLayer.layerColor = QColor(200, 200, 200);
    proposal.operations.push_back(stairLayer);

    double totalLength = stepCount * treadDepth;

    // Left Stringer
    AiOperation leftStr;
    leftStr.type = AiOperationType::CreateLine;
    leftStr.p1 = startPos;
    leftStr.p2 = QPointF(startPos.x(), startPos.y() + totalLength);
    leftStr.layer = QStringLiteral("A-FLOR");
    proposal.operations.push_back(leftStr);

    // Right Stringer
    AiOperation rightStr;
    rightStr.type = AiOperationType::CreateLine;
    rightStr.p1 = QPointF(startPos.x() + stairWidth, startPos.y());
    rightStr.p2 = QPointF(startPos.x() + stairWidth, startPos.y() + totalLength);
    rightStr.layer = QStringLiteral("A-FLOR");
    proposal.operations.push_back(rightStr);

    // Treads
    for (int i = 0; i <= stepCount; ++i) {
        double y = startPos.y() + i * treadDepth;
        AiOperation tread;
        tread.type = AiOperationType::CreateLine;
        tread.p1 = QPointF(startPos.x(), y);
        tread.p2 = QPointF(startPos.x() + stairWidth, y);
        tread.layer = QStringLiteral("A-FLOR");
        proposal.operations.push_back(tread);
    }

    // Direction arrow and text
    AiOperation arrow;
    arrow.type = AiOperationType::CreateLine;
    arrow.p1 = QPointF(startPos.x() + stairWidth / 2.0, startPos.y() + 100.0);
    arrow.p2 = QPointF(startPos.x() + stairWidth / 2.0, startPos.y() + totalLength - 200.0);
    arrow.layer = QStringLiteral("A-FLOR");
    proposal.operations.push_back(arrow);

    AiOperation text;
    text.type = AiOperationType::CreateText;
    text.p1 = QPointF(startPos.x() + stairWidth / 2.0 - 40.0, startPos.y() + totalLength - 100.0);
    text.text = QStringLiteral("UP");
    text.height = 80.0;
    text.layer = QStringLiteral("A-FLOR");
    proposal.operations.push_back(text);

    return proposal;
}

AiProposal AiLocalEngine::generateBoltPattern(int holeCount, double pitchRadius, double holeRadius, const QPointF& center) {
    AiProposal proposal;
    proposal.description = QStringLiteral("Bolt Circle Pattern (%1 holes, pitch radius %2 mm)").arg(holeCount).arg(pitchRadius);

    AiOperation mechLayer;
    mechLayer.type = AiOperationType::AddLayer;
    mechLayer.layerName = QStringLiteral("Mechanical");
    mechLayer.layerColor = QColor(0, 200, 100);
    proposal.operations.push_back(mechLayer);

    // Pitch Circle (Centerline)
    AiOperation pitchCircle;
    pitchCircle.type = AiOperationType::CreateCircle;
    pitchCircle.p1 = center;
    pitchCircle.radius = pitchRadius;
    pitchCircle.layer = QStringLiteral("Mechanical");
    proposal.operations.push_back(pitchCircle);

    // Central bore
    AiOperation centerBore;
    centerBore.type = AiOperationType::CreateCircle;
    centerBore.p1 = center;
    centerBore.radius = pitchRadius * 0.4;
    centerBore.layer = QStringLiteral("Mechanical");
    proposal.operations.push_back(centerBore);

    // Bolt holes
    double stepAngle = 2.0 * M_PI / std::max(1, holeCount);
    for (int i = 0; i < holeCount; ++i) {
        double angle = i * stepAngle;
        double hx = center.x() + pitchRadius * std::cos(angle);
        double hy = center.y() + pitchRadius * std::sin(angle);

        AiOperation hole;
        hole.type = AiOperationType::CreateCircle;
        hole.p1 = QPointF(hx, hy);
        hole.radius = holeRadius;
        hole.layer = QStringLiteral("Mechanical");
        proposal.operations.push_back(hole);
    }

    return proposal;
}

AiProposal AiLocalEngine::generateFromPrompt(const QString& prompt, const CadDocument* doc) {
    QString p = prompt.trimmed();
    QString lower = p.toLower();

    // 1. Architectural Floor Plan Generation
    // e.g. "Create a 10 m x 12 m residential ground floor with 3 bedrooms, kitchen, living room, 2 toilets, staircase, doors, windows and dimensions."
    if (lower.contains("residential") || lower.contains("floor plan") || lower.contains("ground floor") ||
        lower.contains("bhk") || lower.contains("bedroom") ||
        (lower.contains("kitchen") && lower.contains("living")) ||
        (lower.contains("kitchen") && lower.contains("toilet"))) {

        FloorPlanProviderResult fpRes = AiFloorPlanEngine::instance().generatePlan(prompt);
        if (fpRes.success) {
            return fpRes.proposal;
        } else if (fpRes.needsClarification) {
            AiProposal prop;
            prop.description = fpRes.clarificationQuestion;
            prop.prompt = prompt;
            return prop;
        } else if (!fpRes.errorMessage.isEmpty()) {
            AiProposal prop;
            prop.description = QStringLiteral("Floor Plan Error: %1").arg(fpRes.errorMessage);
            prop.prompt = prompt;
            return prop;
        }
    }

    // 2. Direct commands: Clean / Remove Duplicates
    if (lower.contains("clean") || lower.contains("remove duplicate")) {
        return cleanupDocument(doc);
    }

    // 2. Direct commands: Add Dimensions / Auto Dimension
    if (lower.contains("dimension") || lower.contains("add dim")) {
        return dimensionDocument(doc);
    }

    // 3. Room Generation: e.g. "Create a 5m x 4m room", "room 5000 x 3000", "5m by 4m room"
    QRegularExpression roomRe(QStringLiteral(R"(room\s*(?:of\s*)?(\d+(?:\.\d+)?)\s*(m|mm)?\s*(?:x|by|\*)\s*(\d+(?:\.\d+)?)\s*(m|mm)?)"),
                              QRegularExpression::CaseInsensitiveOption);
    auto roomMatch = roomRe.match(lower);
    if (!roomMatch.hasMatch()) {
        // Try inverted pattern: "5m x 4m room"
        QRegularExpression roomRe2(QStringLiteral(R"((\d+(?:\.\d+)?)\s*(m|mm)?\s*(?:x|by|\*)\s*(\d+(?:\.\d+)?)\s*(m|mm)?\s*room)"),
                                   QRegularExpression::CaseInsensitiveOption);
        roomMatch = roomRe2.match(lower);
    }

    if (roomMatch.hasMatch()) {
        double w = roomMatch.captured(1).toDouble();
        QString unit1 = roomMatch.captured(2).toLower();
        double h = roomMatch.captured(3).toDouble();
        QString unit2 = roomMatch.captured(4).toLower();

        // Convert m to mm if unit specified or value < 20 (presumed meters)
        if (unit1 == "m" || (unit1.isEmpty() && w <= 50.0)) w *= 1000.0;
        if (unit2 == "m" || (unit2.isEmpty() && h <= 50.0)) h *= 1000.0;

        double wallThick = 200.0;
        QRegularExpression wallThickRe(QStringLiteral(R"((\d+(?:\.\d+)?)\s*(m|mm)?\s*walls?)"));
        auto wtMatch = wallThickRe.match(lower);
        if (wtMatch.hasMatch()) {
            wallThick = wtMatch.captured(1).toDouble();
            if (wtMatch.captured(2) == "m") wallThick *= 1000.0;
        }

        return generateRoom(w, h, wallThick);
    }

    // 4. Door Generation: "Add a 900 mm door on south wall", "door 800mm", "door 900"
    if (lower.contains("door")) {
        double width = 900.0;
        QRegularExpression doorWidthRe(QStringLiteral(R"((\d+(?:\.\d+)?)\s*(mm|m)?)"));
        auto dwMatch = doorWidthRe.match(lower);
        if (dwMatch.hasMatch()) {
            width = dwMatch.captured(1).toDouble();
            if (dwMatch.captured(2) == "m") width *= 1000.0;
        }

        QPointF pos(1000.0, 0.0);
        if (doc && doc->boundingBox().isValid()) {
            QRectF bbox = doc->boundingBox();
            pos = QPointF(bbox.left() + 500.0, bbox.bottom());
        }

        return generateDoor(width, pos);
    }

    // 5. Window Generation: "Create four windows", "Add 4 windows", "window 1200mm"
    if (lower.contains("window")) {
        int count = 2;
        if (lower.contains("four") || lower.contains("4")) count = 4;
        else if (lower.contains("three") || lower.contains("3")) count = 3;
        else if (lower.contains("two") || lower.contains("2")) count = 2;
        else if (lower.contains("one") || lower.contains("1")) count = 1;

        double width = 1200.0;
        QRegularExpression winWidthRe(QStringLiteral(R"((\d+(?:\.\d+)?)\s*(mm|m)?)"));
        auto wwMatch = winWidthRe.match(lower);
        if (wwMatch.hasMatch()) {
            double val = wwMatch.captured(1).toDouble();
            if (val > 10.0) {
                width = val;
                if (wwMatch.captured(2) == "m") width *= 1000.0;
            }
        }

        return generateWindows(count, width, doc);
    }

    // 6. Staircase Generation: "Create a staircase", "stair 10 steps"
    if (lower.contains("stair")) {
        int steps = 12;
        QRegularExpression stepsRe(QStringLiteral(R"((\d+)\s*steps?)"));
        auto stMatch = stepsRe.match(lower);
        if (stMatch.hasMatch()) {
            steps = stMatch.captured(1).toInt();
        }
        return generateStairs(steps);
    }

    // 7. Bolt Circle / Flange: "bolt circle", "bolt pattern", "flange"
    if (lower.contains("bolt") || lower.contains("flange")) {
        int holes = 6;
        QRegularExpression holesRe(QStringLiteral(R"((\d+)\s*holes?)"));
        auto hMatch = holesRe.match(lower);
        if (hMatch.hasMatch()) {
            holes = hMatch.captured(1).toInt();
        }

        double pitchR = 100.0;
        QRegularExpression radRe(QStringLiteral(R"(radius\s*(\d+(?:\.\d+)?))"));
        auto rMatch = radRe.match(lower);
        if (rMatch.hasMatch()) {
            pitchR = rMatch.captured(1).toDouble();
        }

        return generateBoltPattern(holes, pitchR);
    }

    // 8. Rectangle: "Create a rectangle 500 mm x 300 mm" (README Section 4.1 example)
    QRegularExpression rectRe(QStringLiteral(R"((?:rect|rectangle)\s*(?:of\s*)?(\d+(?:\.\d+)?)\s*(mm|m)?\s*(?:x|by|\*)\s*(\d+(?:\.\d+)?)\s*(mm|m)?)"),
                              QRegularExpression::CaseInsensitiveOption);
    auto rectMatch = rectRe.match(lower);
    if (rectMatch.hasMatch()) {
        double w = rectMatch.captured(1).toDouble();
        if (rectMatch.captured(2) == "m") w *= 1000.0;
        double h = rectMatch.captured(3).toDouble();
        if (rectMatch.captured(4) == "m") h *= 1000.0;

        AiProposal proposal;
        proposal.description = QStringLiteral("Rectangle %1 x %2 mm").arg(w).arg(h);
        proposal.prompt = prompt;

        AiOperation op;
        op.type = AiOperationType::CreateRectangle;
        op.p1 = QPointF(0, 0);
        op.width = w;
        op.height = h;
        op.layer = QStringLiteral("0");
        proposal.operations.push_back(op);
        return proposal;
    }

    // 9. Circle: "circle radius 50", "circle with diameter 100"
    if (lower.contains("circle")) {
        double r = 50.0;
        QRegularExpression rRe(QStringLiteral(R"((?:radius|r)\s*(?:of\s*)?(\d+(?:\.\d+)?))"));
        auto rM = rRe.match(lower);
        if (rM.hasMatch()) {
            r = rM.captured(1).toDouble();
        } else {
            QRegularExpression dRe(QStringLiteral(R"((?:diameter|dia)\s*(?:of\s*)?(\d+(?:\.\d+)?))"));
            auto dM = dRe.match(lower);
            if (dM.hasMatch()) {
                r = dM.captured(1).toDouble() / 2.0;
            }
        }

        AiProposal proposal;
        proposal.description = QStringLiteral("Circle radius %1 mm").arg(r);
        proposal.prompt = prompt;

        AiOperation op;
        op.type = AiOperationType::CreateCircle;
        op.p1 = QPointF(0, 0);
        op.radius = r;
        op.layer = QStringLiteral("0");
        proposal.operations.push_back(op);
        return proposal;
    }

    // 10. Line: "line from 0,0 to 100,50", "line 200mm"
    if (lower.contains("line")) {
        QPointF p1(0, 0);
        QPointF p2(100, 0);

        QRegularExpression lineRe(QStringLiteral(R"(from\s*(-?\d+(?:\.\d+)?)\s*,\s*(-?\d+(?:\.\d+)?)\s*to\s*(-?\d+(?:\.\d+)?)\s*,\s*(-?\d+(?:\.\d+)?))"));
        auto lM = lineRe.match(lower);
        if (lM.hasMatch()) {
            p1 = QPointF(lM.captured(1).toDouble(), lM.captured(2).toDouble());
            p2 = QPointF(lM.captured(3).toDouble(), lM.captured(4).toDouble());
        } else {
            QRegularExpression lenRe(QStringLiteral(R"((\d+(?:\.\d+)?)\s*(mm|m)?)"));
            auto lenM = lenRe.match(lower);
            if (lenM.hasMatch()) {
                double len = lenM.captured(1).toDouble();
                if (lenM.captured(2) == "m") len *= 1000.0;
                p2 = QPointF(len, 0);
            }
        }

        AiProposal proposal;
        proposal.description = QStringLiteral("Line from (%1, %2) to (%3, %4)")
                               .arg(p1.x()).arg(p1.y()).arg(p2.x()).arg(p2.y());
        proposal.prompt = prompt;

        AiOperation op;
        op.type = AiOperationType::CreateLine;
        op.p1 = p1;
        op.p2 = p2;
        op.layer = QStringLiteral("0");
        proposal.operations.push_back(op);
        return proposal;
    }

    // Default fallback: return empty proposal with note
    AiProposal proposal;
    proposal.description = QStringLiteral("Unrecognized prompt: '%1'. Try 'Create a 5m x 4m room', 'Rectangle 500x300', 'Add door', or 'Clean drawing'.").arg(prompt);
    proposal.prompt = prompt;
    return proposal;
}
