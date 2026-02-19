#pragma once
#include <Standard_Handle.hxx>
class TopoDS_Shape;
class TDF_Label;
class XCAFDoc_ShapeTool;
class AIS_InteractiveObject;
class TCollection_ExtendedString;
namespace Util{
namespace Doc
{
    TCollection_ExtendedString GetNameFromLabel(const TDF_Label &label);
    TDF_Label GetLabelFromShape(const TopoDS_Shape &shape,
                                       const Handle(XCAFDoc_ShapeTool) & shapeTool);
} // namespace Doc


namespace Ais
{
    TCollection_ExtendedString GetNameFromAISObject(const Handle(AIS_InteractiveObject)& object);
} 

namespace TopoShape
{
    TopoDS_Shape CreateBoundingBox(const TopoDS_Shape &shape);
    std::string GetShapeTypeString(const TopoDS_Shape& shape);
} // namespace TopoShape

} // namespace Util