#include "SheetMetalCollisionChecker.h"

#include <BRepAlgoAPI_Common.hxx>
#include <BRepBndLib.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <Bnd_Box.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>

#include <algorithm>
#include <cmath>

namespace
{
bool hasTopology(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return false;
    }
    TopExp_Explorer explorer(shape, TopAbs_VERTEX);
    return explorer.More();
}

}

SheetMetalCollisionResult SheetMetalCollisionChecker::check(
    const TopoDS_Shape& connector,
    const SheetMetalConnectorData& data,
    const std::vector<Handle(AIS_InteractiveObject)>& sceneObjects,
    const TopoDS_Shape& ignoredShape)
{
    const auto obstacleMap = SheetMetalObstacleMap::build(
        connector, data, sceneObjects, ignoredShape);
    return check(connector, data, obstacleMap);
}

SheetMetalCollisionResult SheetMetalCollisionChecker::check(
    const TopoDS_Shape& connector,
    const SheetMetalConnectorData& data,
    const SheetMetalObstacleMap& obstacleMap)
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

    for (const auto* entry : obstacleMap.query(connectorBox)) {
        ++result.obstacleCount;
        BRepAlgoAPI_Common common(connector, entry->shape);
        common.Build();
        if (common.IsDone() && hasTopology(common.Shape())) {
            result.hasCollision = true;
            continue;
        }

        BRepExtrema_DistShapeShape distance(connector, entry->shape);
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
