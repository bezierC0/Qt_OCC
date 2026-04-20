#pragma once

#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

/**
 * @brief Pure shape-creation functions - no Qt, no UI, no display.
 *
 * All methods return a TopoDS_Shape. An empty (null) shape is returned on failure.
 * This layer is the only place that directly calls OpenCASCADE builder APIs for
 * the 2-D/3-D primitives used by the create-shape dialogs.
 * Replacing the geometry kernel only requires changing this file.
 */
namespace CoreApi
{
namespace ShapeFactory
{
    TopoDS_Shape makePoint(double x, double y, double z);

    TopoDS_Shape makeLine(const gp_Pnt &p1, const gp_Pnt &p2);

    TopoDS_Shape makeRectangleWire(const gp_Pnt &origin, double width, double height);

    /// Preview helper: rectangle from two diagonal corners in the XY plane at z = p1.Z()
    TopoDS_Shape makeRectangleWireFromCorners(const gp_Pnt &p1, const gp_Pnt &p2);

    TopoDS_Shape makeCircle(const gp_Pnt &center, double radius);

    TopoDS_Shape makeEllipse(const gp_Pnt &center, double nx, double ny, double nz,
                             double majorRadius, double minorRadius);
} // namespace ShapeFactory
} // namespace CoreApi
