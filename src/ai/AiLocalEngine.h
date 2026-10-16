#pragma once

#include <QString>
#include <memory>
#include "ai/AiDsl.h"
#include "ai/AiGeometryAnalyzer.h"
#include "ai/AiRagEngine.h"

class CadDocument;

class AiLocalEngine {
public:
    AiLocalEngine();

    // Natural Language to CAD proposal
    AiProposal generateFromPrompt(const QString& prompt, const CadDocument* doc = nullptr);

    // Analysis
    AiAnalysisReport analyzeDocument(const CadDocument* doc);

    // Auto-clean
    AiProposal cleanupDocument(const CadDocument* doc);

    // Auto-dimension
    AiProposal dimensionDocument(const CadDocument* doc, const std::vector<QUuid>& targetIds = {});

    // RAG
    AiRagEngine* ragEngine() { return &m_ragEngine; }

private:
    AiProposal generateRoom(double width, double height, double wallThickness = 200.0, const QPointF& origin = QPointF(0, 0));
    AiProposal generateDoor(double width = 900.0, const QPointF& position = QPointF(0, 0), const QString& wall = QStringLiteral("south"));
    AiProposal generateWindows(int count, double width = 1200.0, const CadDocument* doc = nullptr);
    AiProposal generateStairs(int stepCount = 12, double treadDepth = 280.0, double stairWidth = 1000.0, const QPointF& startPos = QPointF(0, 0));
    AiProposal generateBoltPattern(int holeCount = 6, double pitchRadius = 100.0, double holeRadius = 8.0, const QPointF& center = QPointF(0, 0));

    AiRagEngine m_ragEngine;
};
