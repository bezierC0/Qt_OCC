#pragma once

#include "SheetMetalConnectorModel.h"

#include <QStringList>

#include <limits>
#include <vector>

struct SheetMetalRouteEvaluation
{
    bool isValid{false};
    double length{0.0};
    double normalAlignment{-std::numeric_limits<double>::infinity()};
    int bendCount{0};
    int sideBendCount{0};
    QStringList failureReasons;
};

class SheetMetalRouteEvaluator
{
public:
    static SheetMetalRouteEvaluation evaluate(
        const std::vector<gp_Pnt>& points,
        const SheetMetalConnectorData& data);

    static bool isBetter(const SheetMetalRouteEvaluation& candidate,
                         std::size_t candidatePointCount,
                         const SheetMetalRouteEvaluation& current,
                         std::size_t currentPointCount,
                         const SheetMetalConnectorData& data);
};
