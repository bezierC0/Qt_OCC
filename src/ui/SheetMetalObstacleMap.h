#pragma once

#include "SheetMetalConnectorModel.h"

#include <AIS_InteractiveObject.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS_Shape.hxx>

#include <vector>

struct SheetMetalObstacle
{
    TopoDS_Shape shape;
    Bnd_Box bounds;
};

struct SheetMetalObstacleMapStats
{
    int sceneObjectCount{0};
    int excludedObjectCount{0};
    int obstacleCount{0};
};

class SheetMetalObstacleMap
{
public:
    static SheetMetalObstacleMap build(
        const TopoDS_Shape& connector,
        const SheetMetalConnectorData& data,
        const std::vector<Handle(AIS_InteractiveObject)>& sceneObjects,
        const TopoDS_Shape& ignoredShape = TopoDS_Shape());

    std::vector<const SheetMetalObstacle*> query(const Bnd_Box& bounds) const;

    const std::vector<SheetMetalObstacle>& obstacles() const { return m_obstacles; }
    const SheetMetalObstacleMapStats& stats() const { return m_stats; }

private:
    std::vector<SheetMetalObstacle> m_obstacles;
    SheetMetalObstacleMapStats m_stats;
};
