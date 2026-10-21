#include "SheetMetalCollisionChecker.h"

#include <AIS_Shape.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBndLib.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <Bnd_Box.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFPrs_AISObject.hxx>

#include <algorithm>
#include <cmath>

namespace
{
TopoDS_Shape objectShape(const Handle(AIS_InteractiveObject)& object)
{
    if (object.IsNull()) {
        return {};
    }

    TopoDS_Shape shape;
    const auto aisShape = Handle(AIS_Shape)::DownCast(object);
    if (!aisShape.IsNull()) {
        shape = aisShape->Shape();
    } else {
        const auto xcafObject = Handle(XCAFPrs_AISObject)::DownCast(object);
        if (!xcafObject.IsNull()) {
            XCAFDoc_ShapeTool::GetShape(xcafObject->GetLabel(), shape);
        }
    }

    if (!shape.IsNull() && object->LocalTransformation().Form() != gp_Identity) {
        shape.Location(TopLoc_Location(object->LocalTransformation()) * shape.Location());
    }
    return shape;
}

bool hasTopology(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return false;
    }
    TopExp_Explorer explorer(shape, TopAbs_VERTEX);
    return explorer.More();
}

bool sameShape(const TopoDS_Shape& first, const TopoDS_Shape& second)
{
    return !first.IsNull() && !second.IsNull()
        && (first.IsSame(second) || first.IsEqual(second) || first.IsPartner(second));
}

TopoDS_Shape removeAllowedContact(const TopoDS_Shape& obstacle,
                                  const SheetMetalConnectorData& data)
{
    TopoDS_Shape checkedShape = obstacle;
    const double allowanceRadius = std::max(data.width, data.thickness) + data.clearance;

    const auto removeEndAllowance = [&](const SheetMetalConnectionEnd& connection) {
        if (!sameShape(obstacle, connection.targetShape) || checkedShape.IsNull()) {
            return;
        }
        BRepPrimAPI_MakeSphere allowance(connection.position, allowanceRadius);
        allowance.Build();
        if (!allowance.IsDone()) {
            return;
        }
        BRepAlgoAPI_Cut cut(checkedShape, allowance.Shape());
        cut.Build();
        if (cut.IsDone()) {
            checkedShape = cut.Shape();
        }
    };

    removeEndAllowance(data.start);
    removeEndAllowance(data.end);
    return checkedShape;
}
}

SheetMetalCollisionResult SheetMetalCollisionChecker::check(
    const TopoDS_Shape& connector,
    const SheetMetalConnectorData& data,
    const std::vector<Handle(AIS_InteractiveObject)>& sceneObjects,
    const TopoDS_Shape& ignoredShape)
{
    SheetMetalCollisionResult result;
    if (connector.IsNull()) {
        result.hasCollision = true;
        result.messages.push_back(QStringLiteral("The connector shape is invalid."));
        return result;
    }

    Bnd_Box connectorBox;
    BRepBndLib::Add(connector, connectorBox);
    connectorBox.Enlarge(std::max(0.0, data.clearance));

    for (const auto& object : sceneObjects) {
        TopoDS_Shape obstacle = objectShape(object);
        if (obstacle.IsNull() || sameShape(obstacle, connector)
            || sameShape(obstacle, ignoredShape)) {
            continue;
        }
        obstacle = removeAllowedContact(obstacle, data);
        if (obstacle.IsNull() || !hasTopology(obstacle)) {
            continue;
        }

        Bnd_Box obstacleBox;
        BRepBndLib::Add(obstacle, obstacleBox);
        if (connectorBox.IsOut(obstacleBox)) {
            continue;
        }

        ++result.obstacleCount;
        BRepAlgoAPI_Common common(connector, obstacle);
        common.Build();
        if (common.IsDone() && hasTopology(common.Shape())) {
            result.hasCollision = true;
            continue;
        }

        BRepExtrema_DistShapeShape distance(connector, obstacle);
        distance.Perform();
        if (!distance.IsDone() || distance.NbSolution() == 0) {
            continue;
        }
        result.minimumDistance = std::min(result.minimumDistance, distance.Value());
        if (distance.Value() + Precision::Confusion() < data.clearance) {
            result.hasClearanceViolation = true;
        }
    }

    if (result.hasCollision) {
        result.messages.push_back(QStringLiteral("The connector intersects another shape."));
    }
    if (result.hasClearanceViolation) {
        result.messages.push_back(QStringLiteral("The connector does not satisfy the minimum clearance."));
    }
    return result;
}
