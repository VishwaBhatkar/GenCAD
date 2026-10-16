#pragma once

#include <vector>
#include <QUuid>
#include "ai/AiDsl.h"

class CadDocument;

class AiDimensionGenerator {
public:
    static AiProposal generateDimensions(const CadDocument* doc,
                                         const std::vector<QUuid>& targetIds = {},
                                         double offset = 20.0);
};
