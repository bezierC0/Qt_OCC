#include "SheetMetalRoutePlanner.h"

#include <Precision.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

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

std::vector<gp_Pnt> makeCandidate(const gp_Pnt& start,
                                  const gp_Pnt& end,
                                  const std::array<int, 3>& axes)
{
    double current[3] = {start.X(), start.Y(), start.Z()};
    const double target[3] = {end.X(), end.Y(), end.Z()};

    std::vector<gp_Pnt> points{start};
    for (const int axis : axes) {
        current[axis] = target[axis];
        points.emplace_back(current[0], current[1], current[2]);
    }
    return removeDuplicates(points);
}

double pathLength(const std::vector<gp_Pnt>& points)
{
    double length = 0.0;
    for (std::size_t index = 1; index < points.size(); ++index) {
        length += points[index - 1].Distance(points[index]);
    }
    return length;
}

double normalAlignment(const std::vector<gp_Pnt>& points,
                       const SheetMetalConnectionEnd& start,
                       const SheetMetalConnectionEnd& end)
{
    if (points.size() < 2) {
        return -std::numeric_limits<double>::infinity();
    }

    gp_Vec first(points.front(), points[1]);
    gp_Vec last(points[points.size() - 2], points.back());
    if (first.SquareMagnitude() <= Precision::SquareConfusion()
        || last.SquareMagnitude() <= Precision::SquareConfusion()) {
        return -std::numeric_limits<double>::infinity();
    }

    first.Normalize();
    last.Normalize();
    return first.Dot(gp_Vec(start.normal)) - last.Dot(gp_Vec(end.normal));
}
}

bool SheetMetalRoutePlanner::computeOrthogonalRoute(SheetMetalConnectorData& data,
                                                    double bendRadius,
                                                    QString* errorMessage)
{
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

    static constexpr std::array<std::array<int, 3>, 6> axisOrders{{
        {{0, 1, 2}}, {{0, 2, 1}}, {{1, 0, 2}},
        {{1, 2, 0}}, {{2, 0, 1}}, {{2, 1, 0}}
    }};

    std::vector<gp_Pnt> bestPoints;
    double bestAlignment = -std::numeric_limits<double>::infinity();
    double bestLength = std::numeric_limits<double>::infinity();

    for (const auto& axes : axisOrders) {
        const auto candidate = makeCandidate(data.start.position, data.end.position, axes);
        const double alignment = normalAlignment(candidate, data.start, data.end);
        const double length = pathLength(candidate);
        const bool betterAlignment = alignment > bestAlignment + Precision::Confusion();
        const bool sameAlignment = std::abs(alignment - bestAlignment) <= Precision::Confusion();
        const bool fewerBends = !bestPoints.empty() && candidate.size() < bestPoints.size();

        if (bestPoints.empty() || betterAlignment
            || (sameAlignment && fewerBends)
            || (sameAlignment && candidate.size() == bestPoints.size()
                && length < bestLength - Precision::Confusion())) {
            bestPoints = candidate;
            bestAlignment = alignment;
            bestLength = length;
        }
    }

    data.routePoints.clear();
    data.routePoints.reserve(bestPoints.size());
    for (std::size_t index = 0; index < bestPoints.size(); ++index) {
        SheetMetalRoutePoint routePoint;
        routePoint.position = bestPoints[index];
        routePoint.bend.radius = bendRadius;
        routePoint.isEndpoint = index == 0 || index + 1 == bestPoints.size();
        data.routePoints.push_back(routePoint);
    }
    return data.routePoints.size() >= 2;
}
