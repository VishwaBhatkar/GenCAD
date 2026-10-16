#pragma once

#include <QString>
#include <QStringList>
#include <memory>
#include "ai/AiFloorPlanSpec.h"

struct FloorPlanValidationResult {
    bool isValid{true};
    bool needsClarification{false};
    QString clarificationQuestion;
    QStringList clarificationOptions;
    QStringList errors;
    QStringList warnings;

    QString errorString() const {
        return errors.join(QStringLiteral("\n"));
    }
};

class AiFloorPlanGenerator {
public:
    AiFloorPlanGenerator() = default;

    // Parses natural language prompt into structured config.
    // Detects ambiguity if critical parameters (e.g. dimensions) are missing.
    static FloorPlanValidationResult parsePrompt(const QString& prompt, FloorPlanConfig* outConfig);

    // Validates a floor plan configuration before generation
    static FloorPlanValidationResult validateConfig(const FloorPlanConfig& config);

    // Validates generated floor plan layout geometry
    static FloorPlanValidationResult validateLayout(const FloorPlanLayout& layout);

    // Deterministic floor plan generation from configuration
    static FloorPlanLayout generate(const FloorPlanConfig& config);

    // High-level: prompt to floor plan layout with full validation & ambiguity checks
    static bool generateFromPrompt(const QString& prompt,
                                   const FloorPlanConfig* baseConfig,
                                   FloorPlanLayout* outLayout,
                                   FloorPlanValidationResult* outValidation);

private:
    static void generateSingleLineWalls(const FloorPlanConfig& config,
                                        const std::vector<FloorPlanRoomSpec>& rooms,
                                        std::vector<FloorPlanWallSegment>& outWalls);

    static void generateDoubleLineWalls(const FloorPlanConfig& config,
                                        const std::vector<FloorPlanRoomSpec>& rooms,
                                        std::vector<FloorPlanWallSegment>& outWalls);

    static void generateDoors(const FloorPlanConfig& config,
                              const std::vector<FloorPlanRoomSpec>& rooms,
                              std::vector<FloorPlanDoorSpec>& outDoors);

    static void generateWindows(const FloorPlanConfig& config,
                                const std::vector<FloorPlanRoomSpec>& rooms,
                                std::vector<FloorPlanWindowSpec>& outWindows);

    static void generateStairs(const FloorPlanConfig& config,
                               const FloorPlanRoomSpec& stairRoom,
                               std::vector<FloorPlanStairSpec>& outStairs);

    static void generateDimensions(const FloorPlanConfig& config,
                                   const std::vector<FloorPlanRoomSpec>& rooms,
                                   std::vector<FloorPlanDimensionSpec>& outDimensions);

    static void generateLabels(const FloorPlanConfig& config,
                               const std::vector<FloorPlanRoomSpec>& rooms,
                               std::vector<FloorPlanLabelSpec>& outLabels);
};
