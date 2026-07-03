#include "ShapeFactory.h"
#include <TopoDS_Shape.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GC_MakeCircle.hxx>
#include <GC_MakeEllipse.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Precision.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColgp_Array1OfPnt.hxx>
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

TopoDS_Shape ShapeFactory::makeArc(const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3)
{
    if (p1.IsEqual(p2, Precision::Confusion()) ||
        p2.IsEqual(p3, Precision::Confusion()) ||
        p1.IsEqual(p3, Precision::Confusion())) return {};

    GC_MakeArcOfCircle arc(p1, p2, p3);
    if (!arc.IsDone()) return {};

    BRepBuilderAPI_MakeEdge edge(arc.Value());
    return edge.IsDone() ? edge.Shape() : TopoDS_Shape{};
}

TopoDS_Shape ShapeFactory::makePolygonWire(const std::vector<gp_Pnt>& points, bool closed)
{
    if (points.size() < 2) return {};

    BRepBuilderAPI_MakePolygon poly;
    for (const auto& point : points) {
        poly.Add(point);
    }
    if (closed) {
        poly.Close();
    }

    return poly.IsDone() ? poly.Wire() : TopoDS_Shape{};
}

TopoDS_Shape ShapeFactory::makeBezierCurve(const std::vector<gp_Pnt>& points)
{
    if (points.size() < 2) return {};

    TColgp_Array1OfPnt poles(1, static_cast<Standard_Integer>(points.size()));
    for (Standard_Integer i = 1; i <= poles.Length(); ++i) {
        poles.SetValue(i, points[static_cast<size_t>(i - 1)]);
    }

    try {
        Handle(Geom_BezierCurve) bezier = new Geom_BezierCurve(poles);
        BRepBuilderAPI_MakeEdge edge(bezier);
        return edge.IsDone() ? edge.Shape() : TopoDS_Shape{};
    } catch (...) {
        return {};
    }
}

TopoDS_Shape ShapeFactory::makeNurbsCurve(const std::vector<gp_Pnt>& points, int degree)
{
    const Standard_Integer n = static_cast<Standard_Integer>(points.size());
    if (degree < 1 || n <= degree) return {};

    TColgp_Array1OfPnt poles(1, n);
    for (Standard_Integer i = 1; i <= n; ++i) {
        poles.SetValue(i, points[static_cast<size_t>(i - 1)]);
    }

    const Standard_Integer nbKnots = n - degree + 1;
    TColStd_Array1OfReal knots(1, nbKnots);
    TColStd_Array1OfInteger mults(1, nbKnots);

    mults.SetValue(1, degree + 1);
    mults.SetValue(nbKnots, degree + 1);
    knots.SetValue(1, 0.0);
    knots.SetValue(nbKnots, 1.0);

    if (nbKnots > 2) {
        const Standard_Real step = 1.0 / static_cast<Standard_Real>(nbKnots - 1);
        for (Standard_Integer i = 2; i < nbKnots; ++i) {
            mults.SetValue(i, 1);
            knots.SetValue(i, (i - 1) * step);
        }
    }

    try {
        Handle(Geom_BSplineCurve) bspline = new Geom_BSplineCurve(poles, knots, mults, degree);
        BRepBuilderAPI_MakeEdge edge(bspline);
        return edge.IsDone() ? edge.Shape() : TopoDS_Shape{};
    } catch (...) {
        return {};
    }
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

TopoDS_Shape ShapeFactory::makeBox(const gp_Pnt& corner, double dx, double dy, double dz)
{
    if (std::abs(dx) < Precision::Confusion() ||
        std::abs(dy) < Precision::Confusion() ||
        std::abs(dz) < Precision::Confusion()) return {};

    BRepPrimAPI_MakeBox box(corner, gp_Pnt(corner.X() + dx, corner.Y() + dy, corner.Z() + dz));
    const TopoDS_Shape shape = box.Shape();
    return shape.IsNull() ? TopoDS_Shape{} : shape;
}

TopoDS_Shape ShapeFactory::makeSphere(const gp_Pnt& center, double radius)
{
    if (radius < Precision::Confusion()) return {};
    BRepPrimAPI_MakeSphere sphere(center, radius);
    const TopoDS_Shape shape = sphere.Shape();
    return shape.IsNull() ? TopoDS_Shape{} : shape;
}

TopoDS_Shape ShapeFactory::makeCylinder(const gp_Pnt& baseCenter, double radius, double height)
{
    if (radius < Precision::Confusion() || std::abs(height) < Precision::Confusion()) return {};
    BRepPrimAPI_MakeCylinder cylinder(gp_Ax2(baseCenter, gp_Dir(0, 0, 1)), radius, height);
    const TopoDS_Shape shape = cylinder.Shape();
    return shape.IsNull() ? TopoDS_Shape{} : shape;
}

TopoDS_Shape ShapeFactory::makeCone(const gp_Pnt& baseCenter, double radius1, double radius2, double height)
{
    if (radius1 < 0.0 || radius2 < 0.0 ||
        (radius1 < Precision::Confusion() && radius2 < Precision::Confusion()) ||
        std::abs(height) < Precision::Confusion()) return {};

    BRepPrimAPI_MakeCone cone(gp_Ax2(baseCenter, gp_Dir(0, 0, 1)), radius1, radius2, height);
    const TopoDS_Shape shape = cone.Shape();
    return shape.IsNull() ? TopoDS_Shape{} : shape;
}

} // namespace CoreApi
