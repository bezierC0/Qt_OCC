#include "ShapeFactory.h"
#include <TopoDS_Shape.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <GC_MakeCircle.hxx>
#include <GC_MakeEllipse.hxx>
#include <Precision.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
#include <cmath>

namespace CoreApi {
ShapeFactory &ShapeFactory::Instance()
{
    static ShapeFactory instance;
    return instance;
}

TopoDS_Shape ShapeFactory::makePoint(double x, double y, double z)
{
    BRepBuilderAPI_MakeVertex v(gp_Pnt(x, y, z));
    return v.IsDone() ? v.Shape() : TopoDS_Shape{};
}

TopoDS_Shape ShapeFactory::makeLine(const gp_Pnt& p1, const gp_Pnt& p2)
{
    if (p1.IsEqual(p2, Precision::Confusion())) return {};
    BRepBuilderAPI_MakeEdge e(p1, p2);
    return e.IsDone() ? e.Shape() : TopoDS_Shape{};
}

TopoDS_Shape ShapeFactory::makeRectangleWire(const gp_Pnt& origin, double width, double height)
{
    if (std::abs(width) < Precision::Confusion() ||
        std::abs(height) < Precision::Confusion()) return {};

    const double x = origin.X(), y = origin.Y(), z = origin.Z();
    gp_Pnt a(x,         y,          z);
    gp_Pnt b(x + width, y,          z);
    gp_Pnt c(x + width, y + height, z);
    gp_Pnt d(x,         y + height, z);

    BRepBuilderAPI_MakeEdge e1(a,b), e2(b,c), e3(c,d), e4(d,a);
    if (!e1.IsDone() || !e2.IsDone() || !e3.IsDone() || !e4.IsDone()) return {};

    BRepBuilderAPI_MakeWire w;
    w.Add(e1.Edge()); w.Add(e2.Edge()); w.Add(e3.Edge()); w.Add(e4.Edge());
    return w.IsDone() ? w.Shape() : TopoDS_Shape{};
}

TopoDS_Shape ShapeFactory::makeRectangleWireFromCorners(const gp_Pnt& p1, const gp_Pnt& p2)
{
    const double dx = p2.X() - p1.X();
    const double dy = p2.Y() - p1.Y();
    if (std::abs(dx) < Precision::Confusion() ||
        std::abs(dy) < Precision::Confusion()) return {};

    gp_Pnt a(p1.X(), p1.Y(), p1.Z());
    gp_Pnt b(p2.X(), p1.Y(), p1.Z());
    gp_Pnt c(p2.X(), p2.Y(), p1.Z());
    gp_Pnt d(p1.X(), p2.Y(), p1.Z());

    BRepBuilderAPI_MakeEdge e1(a,b), e2(b,c), e3(c,d), e4(d,a);
    if (!e1.IsDone() || !e2.IsDone() || !e3.IsDone() || !e4.IsDone()) return {};

    BRepBuilderAPI_MakeWire w;
    w.Add(e1.Edge()); w.Add(e2.Edge()); w.Add(e3.Edge()); w.Add(e4.Edge());
    return w.IsDone() ? w.Shape() : TopoDS_Shape{};
}

TopoDS_Shape ShapeFactory::makeCircle(const gp_Pnt& center, double radius)
{
    if (radius < Precision::Confusion()) return {};
    gp_Circ circ(gp_Ax2(center, gp_Dir(0, 0, 1)), radius);
    BRepBuilderAPI_MakeEdge e(circ);
    return e.IsDone() ? e.Shape() : TopoDS_Shape{};
}

TopoDS_Shape ShapeFactory::makeEllipse(const gp_Pnt& center,
                         double nx, double ny, double nz,
                         double majorRadius, double minorRadius)
{
    if (majorRadius < minorRadius) std::swap(majorRadius, minorRadius);
    if (minorRadius < Precision::Confusion()) return {};

    gp_Dir normal;
    try { normal = gp_Dir(nx, ny, nz); }
    catch (...) { normal = gp_Dir(0, 0, 1); }

    gp_Elips elips(gp_Ax2(center, normal), majorRadius, minorRadius);
    BRepBuilderAPI_MakeEdge e(elips);
    return e.IsDone() ? e.Shape() : TopoDS_Shape{};
}

} // namespace CoreApi
