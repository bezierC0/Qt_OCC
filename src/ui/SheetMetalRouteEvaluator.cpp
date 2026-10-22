#include "SheetMetalRouteEvaluator.h"

#include <Precision.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <cmath>

namespace
{
gp_Vec rotateAroundAxis(const gp_Vec& vector,
                        const gp_Vec& axis,
                        double angle)
{
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    return vector * cosine
        + axis.Crossed(vector) * sine
        + axis * (axis.Dot(vector) * (1.0 - cosine));
}
}

SheetMetalRouteEvaluation SheetMetalRouteEvaluator::evaluate(
    const std::vector<gp_Pnt>& points,
    const SheetMetalConnectorData& data)
{
    SheetMetalRouteEvaluation result;
    if (data.maximumBendCount < 0) {
        result.failureReasons.push_back(
            QStringLiteral("The maximum bend count cannot be negative."));
        return result;
    }
    if (points.size() < 2) {
        result.failureReasons.push_back(
            QStringLiteral("At least two route points are required."));
        return result;
    }

    gp_Vec firstDirection(points[0], points[1]);
    if (firstDirection.SquareMagnitude() <= Precision::SquareConfusion()) {
        result.failureReasons.push_back(
            QStringLiteral("The route contains a zero-length segment."));
        return result;
    }
    firstDirection.Normalize();

    gp_Vec reference(data.upDirection);
    if (std::abs(reference.Dot(firstDirection)) > 0.999) {
        reference = std::abs(firstDirection.Y()) < 0.999
            ? gp_Vec(0.0, 1.0, 0.0)
            : gp_Vec(1.0, 0.0, 0.0);
    }
    gp_Vec widthDirection = reference.Crossed(firstDirection);
    widthDirection.Normalize();
    gp_Vec thicknessDirection = firstDirection.Crossed(widthDirection);
    thicknessDirection.Normalize();

    for (std::size_t index = 1; index < points.size(); ++index) {
        const double segmentLength = points[index - 1].Distance(points[index]);
        if (segmentLength <= Precision::Confusion()) {
            result.failureReasons.push_back(
                QStringLiteral("The route contains a zero-length segment."));
            return result;
        }
        result.length += segmentLength;
    }

    gp_Vec lastDirection(points[points.size() - 2], points.back());
    lastDirection.Normalize();
    result.normalAlignment = firstDirection.Dot(gp_Vec(data.start.normal))
        - lastDirection.Dot(gp_Vec(data.end.normal));

    for (std::size_t index = 1; index + 1 < points.size(); ++index) {
        gp_Vec incoming(points[index - 1], points[index]);
        gp_Vec outgoing(points[index], points[index + 1]);
        incoming.Normalize();
        outgoing.Normalize();

        const double dot = std::clamp(incoming.Dot(outgoing), -1.0, 1.0);
        const double angle = std::acos(dot);
        if (angle <= Precision::Angular()) {
            continue;
        }

        gp_Vec bendAxis = incoming.Crossed(outgoing);
        if (bendAxis.SquareMagnitude() <= Precision::SquareConfusion()) {
            continue;
        }
        bendAxis.Normalize();
        ++result.bendCount;
        if (std::abs(bendAxis.Dot(thicknessDirection)) > 0.999) {
            ++result.sideBendCount;
        }

        widthDirection = rotateAroundAxis(widthDirection, bendAxis, angle);
        thicknessDirection = rotateAroundAxis(thicknessDirection, bendAxis, angle);
        widthDirection.Normalize();
        thicknessDirection.Normalize();
    }

    result.isValid = true;
    return result;
}

bool SheetMetalRouteEvaluator::isBetter(
    const SheetMetalRouteEvaluation& candidate,
    std::size_t candidatePointCount,
    const SheetMetalRouteEvaluation& current,
    std::size_t currentPointCount,
    const SheetMetalConnectorData& data)
{
    if (!candidate.isValid) {
        return false;
    }
    if (!current.isValid) {
        return true;
    }

    if (!data.allowSideBends
        && candidate.sideBendCount != current.sideBendCount) {
        return candidate.sideBendCount < current.sideBendCount;
    }
    if (candidate.normalAlignment
        > current.normalAlignment + Precision::Confusion()) {
        return true;
    }
    if (std::abs(candidate.normalAlignment - current.normalAlignment)
        > Precision::Confusion()) {
        return false;
    }
    if (candidatePointCount != currentPointCount) {
        return candidatePointCount < currentPointCount;
    }
    return candidate.length < current.length - Precision::Confusion();
}
