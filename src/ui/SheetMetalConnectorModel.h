#pragma once

#include <vector>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <TDF_Label.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

enum class SheetMetalJointType
{
    Weld,
    BoltNut
};

struct SheetMetalBend
{
    double radius{5.0};
};

struct SheetMetalBoltNut
{
    int specIndex{0};
    int holeCount{1};
};

struct SheetMetalWeld
{
    int typeIndex{0};
};

struct SheetMetalConnectionEnd
{
    gp_Pnt position;
    gp_Dir normal{0.0, 0.0, 1.0};
    TopoDS_Face face;
    TopoDS_Shape targetShape;
    TDF_Label targetLabel;
    SheetMetalJointType jointType{SheetMetalJointType::Weld};
    SheetMetalBoltNut boltNut;
    SheetMetalWeld weld;
};

struct SheetMetalRoutePoint
{
    gp_Pnt position;
    SheetMetalBend bend;
    bool isEndpoint{false};
};

struct SheetMetalConnectorData
{
    SheetMetalConnectionEnd start;
    SheetMetalConnectionEnd end;
    std::vector<SheetMetalRoutePoint> routePoints;
    double width{40.0};
    double thickness{5.0};
    double clearance{2.0};
    gp_Dir upDirection{0.0, 0.0, 1.0};
};
