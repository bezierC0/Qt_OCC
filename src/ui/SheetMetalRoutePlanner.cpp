#include "SheetMetalRoutePlanner.h"
#include "SheetMetalRouteEvaluator.h"

#include <Precision.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <utility>

namespace
{
std::vector<gp_Pnt> removeDuplicates(const std::vector<gp_Pnt>& points)
{
    std::vector<gp_Pnt> result;
    result.reserve(points.size());
    for (const auto& point : points) {
        if (result.empty() || result.back().Distance(point) > Precision::Confusion()) {
            result.push_back(point);
        }
    }
    return result;
}

std::vector<gp_Pnt> makeStartPlaneRoute(const SheetMetalConnectorData& data)
{
    const gp_Vec startToEnd(data.start.position, data.end.position);
    const gp_Vec normal(data.start.normal);
    const gp_Pnt projectedEnd = data.end.position.Translated(
        -normal * startToEnd.Dot(normal));
    const std::vector<gp_Pnt> points{
        data.start.position,
        projectedEnd,
        data.end.position
    };
    return removeDuplicates(points);
}

void setRoutePoints(SheetMetalConnectorData& data,
                    const std::vector<gp_Pnt>& points,
                    double bendRadius)
{
    data.routePoints.clear();
    data.routePoints.reserve(points.size());
    for (std::size_t index = 0; index < points.size(); ++index) {
        SheetMetalRoutePoint routePoint;
        routePoint.position = points[index];
        routePoint.transition.bend.radius = bendRadius;
        routePoint.isEndpoint = index == 0 || index + 1 == points.size();
        data.routePoints.push_back(routePoint);
    }
}

}

bool SheetMetalRoutePlanner::computeOrthogonalRoute(SheetMetalConnectorData& data,
                                                    double bendRadius,
                                                    QString* errorMessage,
                                                    const RouteValidator& routeValidator)
{
    if (errorMessage) {
        errorMessage->clear();
    }
    if (data.start.position.Distance(data.end.position) <= Precision::Confusion()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The two connection positions must be different.");
        }
        return false;
    }
    if (bendRadius <= Precision::Confusion()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("The bend radius must be greater than zero.");
        }
        return false;
    }

    const auto points = makeStartPlaneRoute(data);
    const auto evaluation = SheetMetalRouteEvaluator::evaluate(points, data);
    if (!evaluation.isValid) {
        if (errorMessage && !evaluation.failureReasons.isEmpty()) {
            *errorMessage = evaluation.failureReasons.front();
        }
        return false;
    }

    auto candidateData = data;
    setRoutePoints(candidateData, points, bendRadius);
    if (candidateData.routePoints.size() == 3
        && (candidateData.maximumBendCount == 0
            || (!candidateData.allowSideBends
                && evaluation.sideBendCount > 0))) {
        candidateData.routePoints[1].transition.type =
            SheetMetalRouteTransitionType::Weld;
    }
    if (routeValidator && !routeValidator(candidateData)) {
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = QStringLiteral(
                "The start-face plane route intersects an obstacle.");
        }
        return false;
    }

    data.routePoints = std::move(candidateData.routePoints);
    return data.routePoints.size() >= 2;
}

bool SheetMetalRoutePlanner::validateRoute(const SheetMetalConnectorData& data,
                                           QString* errorMessage)
{
    std::vector<gp_Pnt> points;
    points.reserve(data.routePoints.size());
    for (const auto& routePoint : data.routePoints) {
        points.push_back(routePoint.position);
    }
    const auto evaluation = SheetMetalRouteEvaluator::evaluate(points, data);
    if (errorMessage) {
        *errorMessage = evaluation.failureReasons.isEmpty()
            ? QString()
            : evaluation.failureReasons.front();
    }
    return evaluation.isValid;
}
