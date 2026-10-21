#include "SheetMetalConnectorBuilder.h"

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GeomFill_Trihedron.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace
{
struct RoundedCorner
{
    bool active{false};
    gp_Pnt entry;
    gp_Pnt middle;
    gp_Pnt exit;
};

double boltHoleRadius(int specIndex)
{
    static constexpr std::array<double, 4> radii{{3.3, 4.5, 5.5, 6.5}};
    const auto index = static_cast<std::size_t>(std::clamp(specIndex, 0, 3));
    return radii[index];
}

bool createRoundedWire(const SheetMetalConnectorData& data,
                       TopoDS_Wire& wire,
                       QString& errorMessage)
{
    const auto& points = data.routePoints;
    if (points.size() < 2) {
        errorMessage = QStringLiteral("At least two route points are required.");
        return false;
    }

    std::vector<RoundedCorner> corners(points.size());
    for (std::size_t index = 1; index + 1 < points.size(); ++index) {
        gp_Vec incoming(points[index - 1].position, points[index].position);
        gp_Vec outgoing(points[index].position, points[index + 1].position);
        const double incomingLength = incoming.Magnitude();
        const double outgoingLength = outgoing.Magnitude();
        if (incomingLength <= Precision::Confusion()
            || outgoingLength <= Precision::Confusion()) {
            errorMessage = QStringLiteral("The route contains a zero-length segment.");
            return false;
        }

        incoming.Normalize();
        outgoing.Normalize();
        const double dot = std::clamp(incoming.Dot(outgoing), -1.0, 1.0);
        const double angle = std::acos(dot);
        if (angle <= Precision::Angular()) {
            continue;
        }
        if (std::abs(std::acos(-1.0) - angle) <= Precision::Angular()) {
            errorMessage = QStringLiteral("The route contains a 180-degree reversal.");
            return false;
        }

        const double radius = points[index].bend.radius;
        const double tangentOffset = radius * std::tan(angle * 0.5);
        if (radius <= Precision::Confusion()
            || tangentOffset >= incomingLength - Precision::Confusion()
            || tangentOffset >= outgoingLength - Precision::Confusion()) {
            errorMessage = QStringLiteral("A bend radius is too large for its adjacent segments.");
            return false;
        }

        auto& corner = corners[index];
        corner.active = true;
        corner.entry = points[index].position.Translated(-incoming * tangentOffset);
        corner.exit = points[index].position.Translated(outgoing * tangentOffset);

        gp_Vec bisector = -incoming + outgoing;
        if (bisector.SquareMagnitude() <= Precision::SquareConfusion()) {
            errorMessage = QStringLiteral("Failed to determine the bend center.");
            return false;
        }
        bisector.Normalize();
        const gp_Pnt center = points[index].position.Translated(
            bisector * (radius / std::sin(angle * 0.5)));
        gp_Vec radialEntry(center, corner.entry);
        gp_Vec radialExit(center, corner.exit);
        gp_Vec radialMiddle = radialEntry + radialExit;
        if (radialMiddle.SquareMagnitude() <= Precision::SquareConfusion()) {
            errorMessage = QStringLiteral("Failed to construct a bend arc.");
            return false;
        }
        radialMiddle.Normalize();
        corner.middle = center.Translated(radialMiddle * radius);
    }

    for (std::size_t index = 0; index + 1 < points.size(); ++index) {
        const gp_Pnt segmentStart = corners[index].active
            ? corners[index].exit
            : points[index].position;
        const gp_Pnt segmentEnd = corners[index + 1].active
            ? corners[index + 1].entry
            : points[index + 1].position;
        gp_Vec original(points[index].position, points[index + 1].position);
        gp_Vec trimmed(segmentStart, segmentEnd);
        if (trimmed.Magnitude() <= Precision::Confusion()
            || trimmed.Dot(original) <= Precision::Confusion()) {
            errorMessage = QStringLiteral("Adjacent bend radii leave no straight segment.");
            return false;
        }
    }

    BRepBuilderAPI_MakeWire wireBuilder;
    gp_Pnt current = points.front().position;
    for (std::size_t index = 1; index + 1 < points.size(); ++index) {
        const auto& corner = corners[index];
        const gp_Pnt lineEnd = corner.active ? corner.entry : points[index].position;
        if (current.Distance(lineEnd) > Precision::Confusion()) {
            BRepBuilderAPI_MakeEdge line(current, lineEnd);
            if (!line.IsDone()) {
                errorMessage = QStringLiteral("Failed to build a route segment.");
                return false;
            }
            wireBuilder.Add(line.Edge());
        }

        if (corner.active) {
            GC_MakeArcOfCircle arc(corner.entry, corner.middle, corner.exit);
            if (!arc.IsDone()) {
                errorMessage = QStringLiteral("Failed to build a bend arc.");
                return false;
            }
            BRepBuilderAPI_MakeEdge arcEdge(arc.Value());
            if (!arcEdge.IsDone()) {
                errorMessage = QStringLiteral("Failed to build a bend edge.");
                return false;
            }
            wireBuilder.Add(arcEdge.Edge());
            current = corner.exit;
        } else {
            current = lineEnd;
        }
    }

    const gp_Pnt& end = points.back().position;
    if (current.Distance(end) > Precision::Confusion()) {
        BRepBuilderAPI_MakeEdge line(current, end);
        if (!line.IsDone()) {
            errorMessage = QStringLiteral("Failed to build the final route segment.");
            return false;
        }
        wireBuilder.Add(line.Edge());
    }

    if (!wireBuilder.IsDone()) {
        errorMessage = QStringLiteral("Failed to build the route wire.");
        return false;
    }
    wire = wireBuilder.Wire();
    return !wire.IsNull();
}

TopoDS_Shape cutBoltHole(const TopoDS_Shape& shape,
                         const SheetMetalConnectionEnd& connection,
                         double width,
                         double thickness)
{
    if (connection.jointType != SheetMetalJointType::BoltNut || shape.IsNull()) {
        return shape;
    }

    const double depth = std::max(width, thickness) * 3.0;
    const gp_Pnt origin = connection.position.Translated(-gp_Vec(connection.normal) * (depth * 0.5));
    BRepPrimAPI_MakeCylinder cylinder(
        gp_Ax2(origin, connection.normal),
        boltHoleRadius(connection.boltNut.specIndex),
        depth);
    cylinder.Build();
    if (!cylinder.IsDone()) {
        return {};
    }

    BRepAlgoAPI_Cut cut(shape, cylinder.Shape());
    cut.Build();
    return cut.IsDone() ? cut.Shape() : TopoDS_Shape();
}
}

SheetMetalBuildResult SheetMetalConnectorBuilder::build(const SheetMetalConnectorData& data)
{
    SheetMetalBuildResult result;
    if (data.width <= Precision::Confusion()
        || data.thickness <= Precision::Confusion()) {
        result.errorMessage = QStringLiteral("Width and thickness must be greater than zero.");
        return result;
    }

    if (!createRoundedWire(data, result.centerLine, result.errorMessage)) {
        return result;
    }

    try {
        const gp_Pnt& origin = data.routePoints.front().position;
        gp_Vec pathDirection(origin, data.routePoints[1].position);
        if (pathDirection.SquareMagnitude() <= Precision::SquareConfusion()) {
            result.errorMessage = QStringLiteral("The first route segment is invalid.");
            return result;
        }
        pathDirection.Normalize();

        gp_Vec reference(data.upDirection);
        if (std::abs(reference.Dot(pathDirection)) > 0.999) {
            reference = std::abs(pathDirection.Y()) < 0.999
                ? gp_Vec(0.0, 1.0, 0.0)
                : gp_Vec(1.0, 0.0, 0.0);
        }

        gp_Vec widthDirection = reference.Crossed(pathDirection);
        widthDirection.Normalize();
        gp_Vec thicknessDirection = pathDirection.Crossed(widthDirection);
        thicknessDirection.Normalize();

        const double halfWidth = data.width * 0.5;
        const double halfThickness = data.thickness * 0.5;
        BRepBuilderAPI_MakePolygon rectangle(
            origin.Translated(widthDirection * halfWidth + thicknessDirection * halfThickness),
            origin.Translated(-widthDirection * halfWidth + thicknessDirection * halfThickness),
            origin.Translated(-widthDirection * halfWidth - thicknessDirection * halfThickness),
            origin.Translated(widthDirection * halfWidth - thicknessDirection * halfThickness),
            Standard_True);
        if (!rectangle.IsDone()) {
            result.errorMessage = QStringLiteral("Failed to build the sheet-metal profile.");
            return result;
        }

        BRepBuilderAPI_MakeFace profile(rectangle.Wire(), Standard_True);
        if (!profile.IsDone()) {
            result.errorMessage = QStringLiteral("Failed to build the sheet-metal profile face.");
            return result;
        }

        BRepOffsetAPI_MakePipe pipe(
            result.centerLine,
            profile.Face(),
            GeomFill_IsCorrectedFrenet,
            Standard_False);
        pipe.Build();
        if (!pipe.IsDone() || pipe.Shape().IsNull()) {
            result.errorMessage = QStringLiteral("Failed to sweep the sheet-metal body.");
            return result;
        }

        result.solid = cutBoltHole(pipe.Shape(), data.start, data.width, data.thickness);
        if (result.solid.IsNull()) {
            result.errorMessage = QStringLiteral("Failed to create the start connection hole.");
            return result;
        }
        result.solid = cutBoltHole(result.solid, data.end, data.width, data.thickness);
        if (result.solid.IsNull()) {
            result.errorMessage = QStringLiteral("Failed to create the end connection hole.");
        }
    } catch (const Standard_Failure& failure) {
        result.solid.Nullify();
        result.errorMessage = QString::fromUtf8(failure.GetMessageString());
    }
    return result;
}
