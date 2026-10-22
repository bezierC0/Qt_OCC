#include "SheetMetalConnectorBuilder.h"
#include "SheetMetalRoutePlanner.h"

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GeomFill_Trihedron.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Pln.hxx>
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

struct RouteFrameAnalysis
{
    std::vector<bool> bends;
    std::vector<bool> sideBends;
    std::vector<gp_Dir> segmentThicknessDirections;
};

struct SheetPiece
{
    std::vector<SheetMetalRoutePoint> points;
    gp_Dir upDirection{0.0, 0.0, 1.0};
    double thicknessOffset{0.0};
};

struct IntermediateConnection
{
    gp_Pnt position;
    gp_Dir direction{1.0, 0.0, 0.0};
    gp_Dir incomingDirection{1.0, 0.0, 0.0};
    gp_Dir outgoingDirection{1.0, 0.0, 0.0};
    gp_Dir thicknessDirection{0.0, 0.0, 1.0};
    SheetMetalRouteTransition transition;
    bool isCorner{false};
};

double boltHoleRadius(int specIndex)
{
    static constexpr std::array<double, 4> radii{{3.3, 4.5, 5.5, 6.5}};
    const auto index = static_cast<std::size_t>(std::clamp(specIndex, 0, 3));
    return radii[index];
}

bool isLapWeld(const SheetMetalConnectionEnd& connection)
{
    return connection.jointType == SheetMetalJointType::Weld
        && connection.weld.typeIndex == 1;
}

double lapOverlapLength(double thickness)
{
    return std::max(10.0, thickness * 2.0);
}

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

bool createRoundedWire(const std::vector<SheetMetalRoutePoint>& points,
                       double minimumBendRadius,
                       TopoDS_Wire& wire,
                       QString& errorMessage)
{
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

        const double radius = points[index].transition.bend.radius;
        if (radius <= minimumBendRadius + Precision::Confusion()) {
            errorMessage = QStringLiteral(
                "A bend radius is too small for the sheet-metal section.");
            return false;
        }
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

bool createPolylineWire(const std::vector<SheetMetalRoutePoint>& points,
                        TopoDS_Wire& wire,
                        QString& errorMessage)
{
    if (points.size() < 2) {
        errorMessage = QStringLiteral("At least two route points are required.");
        return false;
    }

    BRepBuilderAPI_MakeWire wireBuilder;
    for (std::size_t index = 1; index < points.size(); ++index) {
        BRepBuilderAPI_MakeEdge edge(points[index - 1].position,
                                     points[index].position);
        if (!edge.IsDone()) {
            errorMessage = QStringLiteral("Failed to build the route centerline.");
            return false;
        }
        wireBuilder.Add(edge.Edge());
    }
    if (!wireBuilder.IsDone()) {
        errorMessage = QStringLiteral("Failed to build the route centerline.");
        return false;
    }
    wire = wireBuilder.Wire();
    return !wire.IsNull();
}

bool analyzeRouteFrames(const SheetMetalConnectorData& data,
                        RouteFrameAnalysis& analysis,
                        QString& errorMessage)
{
    const auto& points = data.routePoints;
    if (points.size() < 2) {
        errorMessage = QStringLiteral("At least two route points are required.");
        return false;
    }

    analysis.bends.assign(points.size(), false);
    analysis.sideBends.assign(points.size(), false);
    analysis.segmentThicknessDirections.clear();
    analysis.segmentThicknessDirections.reserve(points.size() - 1);

    gp_Vec direction(points[0].position, points[1].position);
    if (direction.SquareMagnitude() <= Precision::SquareConfusion()) {
        errorMessage = QStringLiteral("The route contains a zero-length segment.");
        return false;
    }
    direction.Normalize();

    gp_Vec reference(data.upDirection);
    if (std::abs(reference.Dot(direction)) > 0.999) {
        reference = std::abs(direction.Y()) < 0.999
            ? gp_Vec(0.0, 1.0, 0.0)
            : gp_Vec(1.0, 0.0, 0.0);
    }
    gp_Vec widthDirection = reference.Crossed(direction);
    widthDirection.Normalize();
    gp_Vec thicknessDirection = direction.Crossed(widthDirection);
    thicknessDirection.Normalize();
    analysis.segmentThicknessDirections.emplace_back(thicknessDirection);

    for (std::size_t index = 1; index + 1 < points.size(); ++index) {
        gp_Vec incoming(points[index - 1].position, points[index].position);
        gp_Vec outgoing(points[index].position, points[index + 1].position);
        if (incoming.SquareMagnitude() <= Precision::SquareConfusion()
            || outgoing.SquareMagnitude() <= Precision::SquareConfusion()) {
            errorMessage = QStringLiteral("The route contains a zero-length segment.");
            return false;
        }
        incoming.Normalize();
        outgoing.Normalize();

        const double dot = std::clamp(incoming.Dot(outgoing), -1.0, 1.0);
        const double angle = std::acos(dot);
        if (std::abs(std::acos(-1.0) - angle) <= Precision::Angular()) {
            errorMessage = QStringLiteral("The route contains a 180-degree reversal.");
            return false;
        }
        if (angle > Precision::Angular()) {
            gp_Vec bendAxis = incoming.Crossed(outgoing);
            bendAxis.Normalize();
            analysis.bends[index] = true;
            analysis.sideBends[index] =
                std::abs(bendAxis.Dot(thicknessDirection)) > 0.999;
            widthDirection = rotateAroundAxis(widthDirection, bendAxis, angle);
            thicknessDirection = rotateAroundAxis(
                thicknessDirection, bendAxis, angle);
            widthDirection.Normalize();
            thicknessDirection.Normalize();
        }
        analysis.segmentThicknessDirections.emplace_back(thicknessDirection);
    }
    return true;
}

bool splitRoute(const SheetMetalConnectorData& data,
                const RouteFrameAnalysis& analysis,
                std::vector<SheetPiece>& pieces,
                std::vector<IntermediateConnection>& connections,
                QString& errorMessage)
{
    pieces.clear();
    connections.clear();

    SheetPiece current;
    current.upDirection = analysis.segmentThicknessDirections.front();
    current.points.push_back(data.routePoints.front());
    int bendCount = 0;

    for (std::size_t index = 1; index + 1 < data.routePoints.size(); ++index) {
        const auto& point = data.routePoints[index];
        if (point.transition.type == SheetMetalRouteTransitionType::Bend) {
            if (analysis.bends[index]
                && !data.allowSideBends
                && analysis.sideBends[index]) {
                errorMessage = QStringLiteral(
                    "A side bend requires a Weld or Bolt/Nut transition.");
                return false;
            }
            if (analysis.bends[index]
                && bendCount >= data.maximumBendCount) {
                errorMessage = QStringLiteral(
                    "The maximum bend count is exceeded by one sheet.");
                return false;
            }
            current.points.push_back(point);
            if (analysis.bends[index]) {
                ++bendCount;
            }
            continue;
        }

        gp_Vec incoming(data.routePoints[index - 1].position, point.position);
        gp_Vec outgoing(point.position, data.routePoints[index + 1].position);
        incoming.Normalize();
        outgoing.Normalize();
        const bool isCorner = analysis.bends[index];
        if (isCorner) {
            if (point.transition.type == SheetMetalRouteTransitionType::BoltNut) {
                errorMessage = QStringLiteral(
                    "A bolted corner joint requires an angle bracket.");
                return false;
            }
            if (point.transition.weld.typeIndex != 0) {
                errorMessage = QStringLiteral(
                    "A corner split supports Butt weld only.");
                return false;
            }
        }

        current.points.push_back(point);
        pieces.push_back(current);

        IntermediateConnection connection;
        connection.position = point.position;
        connection.direction = gp_Dir(incoming);
        connection.incomingDirection = gp_Dir(incoming);
        connection.outgoingDirection = gp_Dir(outgoing);
        connection.thicknessDirection =
            analysis.segmentThicknessDirections[index - 1];
        connection.transition = point.transition;
        connection.isCorner = isCorner;
        connections.push_back(connection);

        current = SheetPiece();
        current.upDirection = analysis.segmentThicknessDirections[index];
        current.points.push_back(point);
        bendCount = 0;
    }

    current.points.push_back(data.routePoints.back());
    pieces.push_back(current);

    int thicknessLayer = 0;
    for (std::size_t index = 0; index < connections.size(); ++index) {
        const auto& connection = connections[index];
        const bool overlapJoint = !connection.isCorner
            && (connection.transition.type == SheetMetalRouteTransitionType::BoltNut
                || connection.transition.weld.typeIndex == 1);
        if (overlapJoint) {
            const double overlap = lapOverlapLength(data.thickness);
            const gp_Vec extension(connection.direction);
            pieces[index].points.back().position.Translate(extension * (overlap * 0.5));
            pieces[index + 1].points.front().position.Translate(-extension * (overlap * 0.5));
            thicknessLayer = thicknessLayer == 0 ? 1 : 0;
        }
        pieces[index + 1].thicknessOffset = thicknessLayer * data.thickness;
    }

    if (isLapWeld(data.start)) {
        gp_Vec direction(pieces.front().points[0].position,
                         pieces.front().points[1].position);
        direction.Normalize();
        pieces.front().points.front().position.Translate(
            -direction * lapOverlapLength(data.thickness));
    }
    if (isLapWeld(data.end)) {
        auto& points = pieces.back().points;
        gp_Vec direction(points[points.size() - 2].position,
                         points.back().position);
        direction.Normalize();
        points.back().position.Translate(
            direction * lapOverlapLength(data.thickness));
    }
    return true;
}

bool trimButtWeldCorner(TopoDS_Shape& incomingShape,
                        TopoDS_Shape& outgoingShape,
                        const IntermediateConnection& connection,
                        double probeDistance,
                        QString& errorMessage)
{
    gp_Vec planeNormal(connection.incomingDirection);
    planeNormal += gp_Vec(connection.outgoingDirection);
    if (planeNormal.SquareMagnitude() <= Precision::SquareConfusion()) {
        errorMessage = QStringLiteral("Failed to determine the corner weld plane.");
        return false;
    }
    planeNormal.Normalize();

    BRepBuilderAPI_MakeFace planeFace(
        gp_Pln(connection.position, gp_Dir(planeNormal)));
    if (!planeFace.IsDone()) {
        errorMessage = QStringLiteral("Failed to build the corner weld plane.");
        return false;
    }

    const gp_Pnt incomingProbe = connection.position.Translated(
        -gp_Vec(connection.incomingDirection) * probeDistance);
    const gp_Pnt outgoingProbe = connection.position.Translated(
        gp_Vec(connection.outgoingDirection) * probeDistance);
    BRepPrimAPI_MakeHalfSpace incomingHalf(planeFace.Face(), incomingProbe);
    BRepPrimAPI_MakeHalfSpace outgoingHalf(planeFace.Face(), outgoingProbe);
    incomingHalf.Build();
    outgoingHalf.Build();
    if (!incomingHalf.IsDone() || !outgoingHalf.IsDone()) {
        errorMessage = QStringLiteral("Failed to build the corner weld half-spaces.");
        return false;
    }

    BRepAlgoAPI_Common trimIncoming(incomingShape, incomingHalf.Solid());
    BRepAlgoAPI_Common trimOutgoing(outgoingShape, outgoingHalf.Solid());
    trimIncoming.Build();
    trimOutgoing.Build();
    if (!trimIncoming.IsDone() || trimIncoming.Shape().IsNull()
        || !trimOutgoing.IsDone() || trimOutgoing.Shape().IsNull()) {
        errorMessage = QStringLiteral("Failed to trim the corner weld sheets.");
        return false;
    }

    incomingShape = trimIncoming.Shape();
    outgoingShape = trimOutgoing.Shape();
    return true;
}

bool buildPiece(const SheetPiece& piece,
                double width,
                double thickness,
                TopoDS_Wire& wire,
                TopoDS_Shape& shape,
                QString& errorMessage)
{
    const double minimumBendRadius = 0.5 * std::hypot(width, thickness);
    if (!createRoundedWire(piece.points, minimumBendRadius, wire, errorMessage)) {
        return false;
    }

    const gp_Pnt& origin = piece.points.front().position;
    gp_Vec pathDirection(origin, piece.points[1].position);
    if (pathDirection.SquareMagnitude() <= Precision::SquareConfusion()) {
        errorMessage = QStringLiteral("The first route segment is invalid.");
        return false;
    }
    pathDirection.Normalize();

    gp_Vec reference(piece.upDirection);
    if (std::abs(reference.Dot(pathDirection)) > 0.999) {
        reference = std::abs(pathDirection.Y()) < 0.999
            ? gp_Vec(0.0, 1.0, 0.0)
            : gp_Vec(1.0, 0.0, 0.0);
    }
    gp_Vec widthDirection = reference.Crossed(pathDirection);
    widthDirection.Normalize();
    gp_Vec thicknessDirection = pathDirection.Crossed(widthDirection);
    thicknessDirection.Normalize();
    const gp_Pnt profileOrigin = origin.Translated(
        thicknessDirection * piece.thicknessOffset);

    const double halfWidth = width * 0.5;
    const double halfThickness = thickness * 0.5;
    BRepBuilderAPI_MakePolygon rectangle(
        profileOrigin.Translated(widthDirection * halfWidth + thicknessDirection * halfThickness),
        profileOrigin.Translated(-widthDirection * halfWidth + thicknessDirection * halfThickness),
        profileOrigin.Translated(-widthDirection * halfWidth - thicknessDirection * halfThickness),
        profileOrigin.Translated(widthDirection * halfWidth - thicknessDirection * halfThickness),
        Standard_True);
    if (!rectangle.IsDone()) {
        errorMessage = QStringLiteral("Failed to build the sheet-metal profile.");
        return false;
    }

    BRepBuilderAPI_MakeFace profile(rectangle.Wire(), Standard_True);
    if (!profile.IsDone()) {
        errorMessage = QStringLiteral("Failed to build the sheet-metal profile face.");
        return false;
    }

    BRepOffsetAPI_MakePipe pipe(
        wire, profile.Face(), GeomFill_IsCorrectedFrenet, Standard_False);
    pipe.Build();
    if (!pipe.IsDone() || pipe.Shape().IsNull()) {
        errorMessage = QStringLiteral("Failed to sweep the sheet-metal body.");
        return false;
    }
    shape = pipe.Shape();
    return true;
}

TopoDS_Shape cutHole(const TopoDS_Shape& shape,
                     const gp_Pnt& position,
                     const gp_Dir& direction,
                     int specIndex,
                     double width,
                     double thickness)
{
    if (shape.IsNull()) {
        return {};
    }

    const double depth = std::max(width, thickness) * 3.0;
    const gp_Pnt origin = position.Translated(-gp_Vec(direction) * (depth * 0.5));
    BRepPrimAPI_MakeCylinder cylinder(
        gp_Ax2(origin, direction), boltHoleRadius(specIndex), depth);
    cylinder.Build();
    if (!cylinder.IsDone()) {
        return {};
    }

    BRepAlgoAPI_Cut cut(shape, cylinder.Shape());
    cut.Build();
    return cut.IsDone() ? cut.Shape() : TopoDS_Shape();
}

TopoDS_Shape cutBoltHole(const TopoDS_Shape& shape,
                         const SheetMetalConnectionEnd& connection,
                         double width,
                         double thickness)
{
    if (connection.jointType != SheetMetalJointType::BoltNut || shape.IsNull()) {
        return shape;
    }

    return cutHole(shape,
                   connection.position,
                   connection.normal,
                   connection.boltNut.specIndex,
                   width,
                   thickness);
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
    if (!SheetMetalRoutePlanner::validateRoute(data, &result.errorMessage)) {
        return result;
    }
    if (!createPolylineWire(data.routePoints,
                            result.centerLine,
                            result.errorMessage)) {
        return result;
    }

    try {
        RouteFrameAnalysis analysis;
        if (!analyzeRouteFrames(data, analysis, result.errorMessage)) {
            return result;
        }
        std::vector<SheetPiece> pieceDefinitions;
        std::vector<IntermediateConnection> connections;
        if (!splitRoute(data,
                        analysis,
                        pieceDefinitions,
                        connections,
                        result.errorMessage)) {
            return result;
        }

        result.pieces.reserve(pieceDefinitions.size());
        for (const auto& definition : pieceDefinitions) {
            TopoDS_Wire pieceWire;
            TopoDS_Shape pieceShape;
            if (!buildPiece(definition,
                            data.width,
                            data.thickness,
                            pieceWire,
                            pieceShape,
                            result.errorMessage)) {
                result.pieces.clear();
                return result;
            }
            result.pieces.push_back(pieceShape);
        }

        for (std::size_t index = 0; index < connections.size(); ++index) {
            const auto& connection = connections[index];
            if (connection.isCorner
                && connection.transition.type == SheetMetalRouteTransitionType::Weld
                && !trimButtWeldCorner(result.pieces[index],
                                       result.pieces[index + 1],
                                       connection,
                                       std::max(data.width, data.thickness),
                                       result.errorMessage)) {
                result.pieces.clear();
                return result;
            }
        }

        result.pieces.front() = cutBoltHole(
            result.pieces.front(), data.start, data.width, data.thickness);
        if (result.pieces.front().IsNull()) {
            result.errorMessage = QStringLiteral("Failed to create the start connection hole.");
            result.pieces.clear();
            return result;
        }
        result.pieces.back() = cutBoltHole(
            result.pieces.back(), data.end, data.width, data.thickness);
        if (result.pieces.back().IsNull()) {
            result.errorMessage = QStringLiteral("Failed to create the end connection hole.");
            result.pieces.clear();
            return result;
        }

        for (std::size_t index = 0; index < connections.size(); ++index) {
            const auto& connection = connections[index];
            if (connection.transition.type == SheetMetalRouteTransitionType::BoltNut) {
                result.pieces[index] = cutHole(
                    result.pieces[index],
                    connection.position,
                    connection.thicknessDirection,
                    connection.transition.boltNut.specIndex,
                    data.width,
                    data.thickness);
                result.pieces[index + 1] = cutHole(
                    result.pieces[index + 1],
                    connection.position,
                    connection.thicknessDirection,
                    connection.transition.boltNut.specIndex,
                    data.width,
                    data.thickness);
                if (result.pieces[index].IsNull()
                    || result.pieces[index + 1].IsNull()) {
                    result.errorMessage = QStringLiteral(
                        "Failed to create an intermediate connection hole.");
                    result.pieces.clear();
                    return result;
                }
            }
        }

        if (result.pieces.size() == 1) {
            result.solid = result.pieces.front();
        } else {
            BRep_Builder builder;
            TopoDS_Compound compound;
            builder.MakeCompound(compound);
            for (const auto& piece : result.pieces) {
                builder.Add(compound, piece);
            }
            result.solid = compound;
        }
    } catch (const Standard_Failure& failure) {
        result.solid.Nullify();
        result.pieces.clear();
        result.errorMessage = QString::fromUtf8(failure.GetMessageString());
    }
    return result;
}
