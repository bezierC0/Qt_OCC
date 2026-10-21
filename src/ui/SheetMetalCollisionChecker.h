#pragma once

#include "SheetMetalConnectorModel.h"

#include <AIS_InteractiveObject.hxx>
#include <QStringList>
#include <TopoDS_Shape.hxx>

#include <limits>
#include <vector>

struct SheetMetalCollisionResult
{
    bool hasCollision{false};
    bool hasClearanceViolation{false};
    int obstacleCount{0};
    double minimumDistance{std::numeric_limits<double>::infinity()};
    QStringList messages;

    bool isAcceptable() const { return !hasCollision && !hasClearanceViolation; }
};

class SheetMetalCollisionChecker
{
public:
    static SheetMetalCollisionResult check(
        const TopoDS_Shape& connector,
        const SheetMetalConnectorData& data,
        const std::vector<Handle(AIS_InteractiveObject)>& sceneObjects,
        const TopoDS_Shape& ignoredShape = TopoDS_Shape());
};
