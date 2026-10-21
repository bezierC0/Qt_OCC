#pragma once

#include "SheetMetalConnectorModel.h"

#include <QString>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>

struct SheetMetalBuildResult
{
    TopoDS_Wire centerLine;
    TopoDS_Shape solid;
    QString errorMessage;

    bool isValid() const { return !centerLine.IsNull() && !solid.IsNull(); }
};

class SheetMetalConnectorBuilder
{
public:
    static SheetMetalBuildResult build(const SheetMetalConnectorData& data);
};
