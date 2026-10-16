#pragma once

#include <vector>
#include <map>
#include <QString>
#include <QStringList>
#include <QUuid>
#include <QRectF>
#include <QJsonObject>
#include <QJsonArray>
#include "ai/AiDsl.h"

class CadDocument;
class CadEntity;
class LineEntity;

struct AiDuplicateIssue {
    QUuid primaryId;
    QUuid duplicateId;
    QString entityType;
    QString description;
};

struct AiOverlapIssue {
    QUuid entityId1;
    QUuid entityId2;
    QString description;
};

struct AiLayerIssue {
    QUuid entityId;
    QString currentLayer;
    QString issueDescription;
};

struct AiAnalysisReport {
    int totalEntities{0};
    std::map<QString, int> entityCountsByType;
    QRectF boundingBox;
    double totalWireLength{0.0};

    std::vector<AiDuplicateIssue> duplicates;
    std::vector<AiOverlapIssue> overlaps;
    std::vector<AiLayerIssue> layerIssues;
    int openEndpointsCount{0};
    int missingDimensionsCount{0};
    QStringList suggestions;

    bool hasIssues() const {
        return !duplicates.empty() || !overlaps.empty() || !layerIssues.empty() || missingDimensionsCount > 0;
    }

    QJsonObject toJson() const;
    QString summaryText() const;
};

class AiGeometryAnalyzer {
public:
    static AiAnalysisReport analyze(const CadDocument* doc, double tolerance = 1e-3);
    static AiProposal generateCleanupProposal(const CadDocument* doc, double tolerance = 1e-3);

private:
    static bool arePointsEqual(const QPointF& p1, const QPointF& p2, double tol = 1e-3);
    static bool areLinesColinearAndOverlapping(const LineEntity* l1, const LineEntity* l2, double tol = 1e-3);
};
