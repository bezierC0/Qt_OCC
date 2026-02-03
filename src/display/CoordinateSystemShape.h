#pragma once

#include <Standard_Handle.hxx>
#include <Standard_Real.hxx>

class gp_Ax2;
class AIS_InteractiveContext;
class Geom_Axis2Placement;
class AIS_Trihedron;
/**
 * @brief Helper class to display a coordinate system (Trihedron) shape in the 3D view.
 *
 */
class CoordinateSystemShape
{
public:
    CoordinateSystemShape();
    CoordinateSystemShape(const Standard_Real axisXLength,const Standard_Real axisYLength,const Standard_Real axisZLength);
    virtual ~CoordinateSystemShape();

    void Display(const Handle(AIS_InteractiveContext)& context);

    void Remove(const Handle(AIS_InteractiveContext)& context);

    void SetLocation(const gp_Ax2& system, const Handle(AIS_InteractiveContext)& context);

private:
    void Init();

private:
    Handle(AIS_Trihedron)           m_trihedron;
    Handle(Geom_Axis2Placement)     m_axis;
    double                          m_axisXLength;
    double                          m_axisYLength;
    double                          m_axisZLength;
};
