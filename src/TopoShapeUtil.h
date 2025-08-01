#pragma once
class TopoDS_Shape;
namespace TopoShape
{

class Util
{
public:
    static TopoDS_Shape CreateBoundingBoxWireframe(const TopoDS_Shape& shape);
};
} // namespace View