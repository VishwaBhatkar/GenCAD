#pragma once

#include <QString>
#include <QPointF>
#include <QUuid>
#include <QColor>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>

enum class AiOperationType {
    CreateLine,
    CreateCircle,
    CreateRectangle,
    CreateArc,
    CreatePolyline,
    CreateText,
    CreateDimension,
    ModifyMove,
    ModifyRotate,
    ModifyMirror,
    DeleteEntity,
    AddLayer,
    SetEntityLayer,
    Unknown
};

struct AiOperation {
    AiOperationType type{AiOperationType::Unknown};
    QUuid targetId; // For modify or delete operations

    // Geometry attributes
    QPointF p1{0.0, 0.0}; // Start, topLeft, or p1
    QPointF p2{0.0, 0.0}; // End, bottomRight, or p2
    QPointF p3{0.0, 0.0}; // dimPoint or center
    double radius{0.0};
    double width{0.0};
    double height{0.0};
    double startAngle{0.0}; // Degrees
    double sweepAngle{0.0}; // Degrees
    std::vector<QPointF> points; // For polyline
    bool closed{false};

    // Text attributes
    QString text;
    double rotation{0.0}; // Degrees

    // Transform attributes
    QPointF delta{0.0, 0.0};
    QPointF origin{0.0, 0.0};
    double angleDeg{0.0};

    // Layer and styling
    QString layer{QStringLiteral("0")};
    QColor color{QColor()};
    bool hasCustomColor{false};

    // Layer creation
    QString layerName;
    QColor layerColor{QColor(220, 220, 220)};

    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject& json, AiOperation* outOp, QString* errorMessage = nullptr);
    static QString operationTypeToString(AiOperationType type);
    static AiOperationType stringToOperationType(const QString& str);
};

struct AiProposal {
    QString description;
    QString prompt;
    std::vector<AiOperation> operations;

    int createdCount() const;
    int modifiedCount() const;
    int deletedCount() const;
    int dimensionCount() const;
    QString summaryText() const;

    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject& json, AiProposal* outProposal, QString* errorMessage = nullptr);
    static bool fromJsonString(const QString& jsonString, AiProposal* outProposal, QString* errorMessage = nullptr);
};
