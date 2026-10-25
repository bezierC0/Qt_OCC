#include "SheetMetalObstacleMap.h"

#include <AIS_Shape.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBndLib.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFPrs_AISObject.hxx>

#include <algorithm>
#include <utility>

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

double allowedContactRadius(const SheetMetalConnectionEnd& connection,
                            const SheetMetalConnectorData& data)
{
    double radius = std::max(data.width, data.thickness);
    if (connection.jointType == SheetMetalJointType::Weld
        && connection.weld.typeIndex == 1) {
        radius = std::max(radius, std::max(10.0, data.thickness * 2.0));
    }
    return radius + data.clearance;
}

TopoDS_Shape removeAllowedContact(const TopoDS_Shape& obstacle,
                                  const SheetMetalConnectorData& data)
{
    TopoDS_Shape checkedShape = obstacle;

    const auto removeEndAllowance = [&](const SheetMetalConnectionEnd& connection) {
        if (!sameShape(obstacle, connection.targetShape) || checkedShape.IsNull()) {
            return;
        }
        BRepPrimAPI_MakeSphere allowance(
            connection.position, allowedContactRadius(connection, data));
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

SheetMetalObstacleMap SheetMetalObstacleMap::build(
    const TopoDS_Shape& connector,
    const SheetMetalConnectorData& data,
    const std::vector<Handle(AIS_InteractiveObject)>& sceneObjects,
    const TopoDS_Shape& ignoredShape)
{
    SheetMetalObstacleMap result;
    result.m_stats.sceneObjectCount = static_cast<int>(sceneObjects.size());
    result.m_obstacles.reserve(sceneObjects.size());

    for (const auto& object : sceneObjects) {
        TopoDS_Shape obstacle = objectShape(object);
        if (obstacle.IsNull() || sameShape(obstacle, connector)
            || sameShape(obstacle, ignoredShape)) {
            ++result.m_stats.excludedObjectCount;
            continue;
        }

        obstacle = removeAllowedContact(obstacle, data);
        if (obstacle.IsNull() || !hasTopology(obstacle)) {
            ++result.m_stats.excludedObjectCount;
            continue;
        }

        SheetMetalObstacle entry;
        entry.shape = obstacle;
        BRepBndLib::Add(entry.shape, entry.bounds);
        result.m_obstacles.push_back(std::move(entry));
    }

    result.m_stats.obstacleCount = static_cast<int>(result.m_obstacles.size());
    return result;
}

std::vector<const SheetMetalObstacle*> SheetMetalObstacleMap::query(
    const Bnd_Box& bounds) const
{
    std::vector<const SheetMetalObstacle*> result;
    result.reserve(m_obstacles.size());
    for (const auto& obstacle : m_obstacles) {
        if (!bounds.IsOut(obstacle.bounds)) {
            result.push_back(&obstacle);
        }
    }
    return result;
}
