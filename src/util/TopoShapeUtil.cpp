#include "TopoShapeUtil.h"
#include <TopoDS_Shape.hxx>
#include <Standard_TypeDef.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

TopoDS_Shape TopoShape::Util::CreateBoundingBox(const TopoDS_Shape &shape)
{
    Bnd_Box box;
    BRepBndLib::Add(shape, box);
    
    if (box.IsVoid()) {
        return TopoDS_Shape(); 
    }
    
    Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    
    TopoDS_Shape boundingBoxShape = BRepPrimAPI_MakeBox(
        gp_Pnt(xmin, ymin, zmin),
        gp_Pnt(xmax, ymax, zmax)
    ).Shape();

    return boundingBoxShape;
}