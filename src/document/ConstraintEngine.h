#pragma once

#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>
#include <memory>
#include <QString>

class CadDocument;

enum class ConstraintType {
    Horizontal,
    Vertical,
    FixedLength,
    Coincident,
    Parallel,
    Perpendicular
};

struct CadConstraint {
    QUuid id;
    ConstraintType type;
    QUuid entityId1;
    QUuid entityId2;
    double paramValue{0.0}; // e.g. length or angle value

    QJsonObject toJson() const;
    static CadConstraint fromJson(const QJsonObject& json);
};

class ConstraintEngine {
public:
    explicit ConstraintEngine(CadDocument* doc);

    const std::vector<CadConstraint>& constraints() const { return m_constraints; }
    void addConstraint(const CadConstraint& constraint);
    bool removeConstraint(const QUuid& id);
    void clear();

    // Constraint enforcement
    bool applyConstraints();

    QJsonArray toJson() const;
    bool fromJson(const QJsonArray& array);

    static QString constraintTypeToString(ConstraintType type);
    static ConstraintType stringToConstraintType(const QString& str, bool* ok = nullptr);

private:
    CadDocument* m_document{nullptr};
    std::vector<CadConstraint> m_constraints;
};
