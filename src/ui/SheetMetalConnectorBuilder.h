#pragma once

#include "SheetMetalConnectorModel.h"

#include <QString>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>

#include <vector>

struct SheetMetalBuildResult
{
    TopoDS_Wire centerLine;
    TopoDS_Shape solid;
    std::vector<TopoDS_Shape> pieces;
    QString errorMessage;

    bool isValid() const { return !centerLine.IsNull() && !solid.IsNull(); }
};

class SheetMetalConnectorBuilder
{
public:
    static SheetMetalBuildResult build(const SheetMetalConnectorData& data);
};
