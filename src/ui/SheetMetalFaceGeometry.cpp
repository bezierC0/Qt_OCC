#include "SheetMetalFaceGeometry.h"

#include <BRep_Tool.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomLProp_SLProps.hxx>
#include <Geom_Surface.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopAbs_State.hxx>

#include <cmath>
#include <limits>

namespace
{
bool isOnFace(const TopoDS_Face& face, const gp_Pnt& point)
{
    BRepClass_FaceClassifier classifier(face, point, Precision::Confusion());
    return classifier.State() == TopAbs_IN || classifier.State() == TopAbs_ON;
}
}

bool SheetMetalFaceGeometry::centerPointAndNormal(const TopoDS_Face& face,
                                                   gp_Pnt& point,
                                                   gp_Dir& normal)
{
    if (face.IsNull()) {
        return false;
    }

    GProp_GProps properties;
    BRepGProp::SurfaceProperties(face, properties);
    const gp_Pnt center = properties.CentreOfMass();
    if (projectPoint(face, center, point)
        && normalAtPoint(face, point, normal)) {
        return true;
    }

    const Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
    if (surface.IsNull()) {
        return false;
    }

    Standard_Real uMin = 0.0;
    Standard_Real uMax = 0.0;
    Standard_Real vMin = 0.0;
    Standard_Real vMax = 0.0;
    BRepTools::UVBounds(face, uMin, uMax, vMin, vMax);
    if (!std::isfinite(uMin) || !std::isfinite(uMax)
        || !std::isfinite(vMin) || !std::isfinite(vMax)) {
        return false;
    }

    constexpr int sampleCount = 11;
    double bestDistance = std::numeric_limits<double>::max();
    gp_Pnt bestPoint;
    bool found = false;
    for (int uIndex = 0; uIndex < sampleCount; ++uIndex) {
        const double u = uMin + (uMax - uMin) * (uIndex + 0.5) / sampleCount;
        for (int vIndex = 0; vIndex < sampleCount; ++vIndex) {
            const double v = vMin + (vMax - vMin) * (vIndex + 0.5) / sampleCount;
            const gp_Pnt candidate = surface->Value(u, v);
            if (!isOnFace(face, candidate)) {
                continue;
            }
            const double distance = center.SquareDistance(candidate);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestPoint = candidate;
                found = true;
            }
        }
    }

    if (!found || !normalAtPoint(face, bestPoint, normal)) {
        return false;
    }
    point = bestPoint;
    return true;
}

bool SheetMetalFaceGeometry::projectPoint(const TopoDS_Face& face,
                                           const gp_Pnt& referencePoint,
                                           gp_Pnt& point)
{
    if (face.IsNull()) {
        return false;
    }

    const Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
    if (surface.IsNull()) {
        return false;
    }

    GeomAPI_ProjectPointOnSurf projector(referencePoint, surface);
    if (projector.NbPoints() == 0) {
        return false;
    }

    const gp_Pnt projectedPoint = projector.NearestPoint();
    if (!isOnFace(face, projectedPoint)) {
        return false;
    }
    point = projectedPoint;
    return true;
}

bool SheetMetalFaceGeometry::normalAtPoint(const TopoDS_Face& face,
                                            const gp_Pnt& point,
                                            gp_Dir& normal)
{
    const Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
    if (surface.IsNull()) {
        return false;
    }

    GeomAPI_ProjectPointOnSurf projector(point, surface);
    if (projector.NbPoints() == 0) {
        return false;
    }

    Standard_Real u = 0.0;
    Standard_Real v = 0.0;
    projector.LowerDistanceParameters(u, v);
    GeomLProp_SLProps surfaceProperties(
        surface, u, v, 1, Precision::Confusion());
    if (!surfaceProperties.IsNormalDefined()) {
        return false;
    }

    normal = surfaceProperties.Normal();
    if (face.Orientation() == TopAbs_REVERSED) {
        normal.Reverse();
    }
    return true;
}
