#pragma once

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Face.hxx>

class SheetMetalFaceGeometry
{
public:
    static bool centerPointAndNormal(const TopoDS_Face& face,
                                     gp_Pnt& point,
                                     gp_Dir& normal);

    static bool projectPoint(const TopoDS_Face& face,
                             const gp_Pnt& referencePoint,
                             gp_Pnt& point);

private:
    static bool normalAtPoint(const TopoDS_Face& face,
                              const gp_Pnt& point,
                              gp_Dir& normal);
};
