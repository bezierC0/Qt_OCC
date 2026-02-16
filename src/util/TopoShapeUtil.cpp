#include "TopoShapeUtil.h"
#include <TopoDS_Shape.hxx>
#include <Standard_TypeDef.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <XCAFPrs_AISObject.hxx>
#include <TDataStd_Name.hxx>
#include <TDF_Label.hxx>
#include <XCAFDoc_ShapeTool.hxx>

TCollection_ExtendedString Util::Doc::GetNameFromLabel(const TDF_Label& label)
{
    TCollection_ExtendedString name;
    Handle(TDataStd_Name) nameAttr;
    if (label.FindAttribute(TDataStd_Name::GetID(), nameAttr)) {
        name = nameAttr->Get();
    }
    return name;
}

TDF_Label Util::Doc::GetLabelFromShape(const TopoDS_Shape& shape, const Handle(XCAFDoc_ShapeTool)& shapeTool)
{
    TDF_Label label;
    if (!shapeTool.IsNull() && !shape.IsNull()) {
        shapeTool->FindShape(shape, label);
    }
    return label;
}


TCollection_ExtendedString Util::Ais::GetNameFromAISObject(const Handle(AIS_InteractiveObject)& object)
{
    TCollection_ExtendedString objectName;
    if (!object.IsNull() && object->IsKind(STANDARD_TYPE(XCAFPrs_AISObject))) {
        Handle(XCAFPrs_AISObject) xcafObj = Handle(XCAFPrs_AISObject)::DownCast(object);
        TDF_Label label = xcafObj->GetLabel();
        objectName = Util::Doc::GetNameFromLabel(label);
    }
    return objectName;
}


TopoDS_Shape Util::TopoShape::CreateBoundingBox(const TopoDS_Shape &shape)
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

// Helper to determine shape type string
std::string Util::TopoShape::GetShapeTypeString(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) return "Shape";
    switch (shape.ShapeType())
    {
        case TopAbs_COMPOUND:       return "Compound";
        case TopAbs_COMPSOLID:      return "CompSolid";
        case TopAbs_SOLID:          return "Solid";
        case TopAbs_SHELL:          return "Shell";
        case TopAbs_FACE:           return "Face";
        case TopAbs_WIRE:           return "Wire";
        case TopAbs_EDGE:           return "Edge";
        case TopAbs_VERTEX:         return "Vertex";
        case TopAbs_SHAPE:          return "Shape";
        default:                    return "Unknown";
    }
}
