#pragma once

#include <QStringList>
#include <cmath>
#include "ai/AiDsl.h"

class CadDocument;

struct AiValidationResult {
    bool isValid{true};
    QStringList errors;
    QStringList warnings;

    QString errorString() const {
        return errors.join(QStringLiteral("\n"));
    }
};

class AiValidator {
public:
    static AiValidationResult validateProposal(const AiProposal& proposal, const CadDocument* doc = nullptr);
    static AiValidationResult validateOperation(const AiOperation& op, const CadDocument* doc = nullptr);

private:
    static bool isPointValid(const QPointF& pt);
    static double distance(const QPointF& p1, const QPointF& p2);
};
