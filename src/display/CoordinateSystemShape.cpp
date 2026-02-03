#include "CoordinateSystemShape.h"
#include <AIS_Trihedron.hxx>
#include <AIS_InteractiveContext.hxx>
#include <Prs3d_DatumAspect.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_DatumParts.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <Geom_Axis2Placement.hxx>
#include <gp_Ax2.hxx>
#include <gp.hxx>

CoordinateSystemShape::CoordinateSystemShape()
    : m_axisXLength(20.0),
    m_axisYLength(m_axisXLength),
    m_axisZLength(m_axisXLength)
{
    Init();
}

CoordinateSystemShape::CoordinateSystemShape(const Standard_Real axisXLength,
                                             const Standard_Real axisYLength,
                                             const Standard_Real axisZLength)
    : m_axisXLength(axisXLength),
    m_axisYLength(axisYLength),
    m_axisZLength(axisZLength)
{
    Init();
}

CoordinateSystemShape::~CoordinateSystemShape()
{
    if( !m_trihedron.IsNull() ){
        m_trihedron.Nullify();
        m_trihedron = nullptr;
    }
    
    if( !m_axis.IsNull() ){
        m_axis.Nullify();
        m_axis = nullptr;
    }
}

void CoordinateSystemShape::Display(const Handle(AIS_InteractiveContext)& context)
{
    if (context.IsNull()) return;
    if (!context->IsDisplayed(m_trihedron)) {
        context->Display(m_trihedron, Standard_True);
    }
}

void CoordinateSystemShape::Remove(const Handle(AIS_InteractiveContext)& context)
{
    if (context.IsNull()) return;
    if (context->IsDisplayed(m_trihedron)) {
        context->Remove(m_trihedron, Standard_True);
    }
}

void CoordinateSystemShape::SetLocation(const gp_Ax2& system, const Handle(AIS_InteractiveContext)& context)
{
    m_axis->SetAx2(system);
    // Notify AIS that the component has changed
    m_trihedron->SetComponent(m_axis);
    
    if (!context.IsNull() && context->IsDisplayed(m_trihedron)) {
        context->Redisplay(m_trihedron, Standard_True);
    }
}

void CoordinateSystemShape::Init()
{
    // Default system at origin
    m_axis = new Geom_Axis2Placement(gp::XOY());
    m_trihedron = new AIS_Trihedron(m_axis);

    // Setup Appearance
    // Prs3d_DM_Shaded : ShadingAspect 
    // Prs3d_DM_WireFrame : LineAspect
    m_trihedron->SetDatumDisplayMode(Prs3d_DM_WireFrame);
    
    // Configure Colors: X=Red, Y=Green, Z=Blue
    // Ensure we have Attributes (usually created by default)
    if (m_trihedron->Attributes().IsNull()) {
        m_trihedron->SetAttributes(new Prs3d_Drawer());
    }
    
    Handle(Prs3d_DatumAspect) aspect = m_trihedron->Attributes()->DatumAspect();
    if (aspect.IsNull()) {
        aspect = new Prs3d_DatumAspect();
        m_trihedron->Attributes()->SetDatumAspect(aspect);
    }

    // Set axis Length
    aspect->SetAxisLength(m_axisXLength, m_axisYLength, m_axisZLength);
    
    // Disable arrows
    aspect->SetDrawArrows(Standard_False);

    // X Axis - Red
    aspect->LineAspect(Prs3d_DP_XAxis)->SetColor(Quantity_NOC_RED);
    
    // Y Axis - Green
    aspect->LineAspect(Prs3d_DP_YAxis)->SetColor(Quantity_NOC_GREEN);

    // Z Axis - Blue
    aspect->LineAspect(Prs3d_DP_ZAxis)->SetColor(Quantity_NOC_BLUE);
}

