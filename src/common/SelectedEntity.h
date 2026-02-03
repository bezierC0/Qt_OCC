#pragma once
#include <Standard_WarningsDisable.hxx>
#include <Standard_WarningsRestore.hxx>

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewController.hxx>
#include <TopAbs_ShapeEnum.hxx>
namespace View
{
class SelectedEntity {
public:
    SelectedEntity(Handle(AIS_InteractiveObject) parentObject,TopoDS_Shape selectedShape)
        : m_parentObject(parentObject),m_selectedShape(selectedShape)
    {
    }

    Handle(AIS_InteractiveObject) GetParentInteractiveObject(){
        return m_parentObject;
    }
    TopoDS_Shape GetSelectedShape(){
        return m_selectedShape;
    }
    Handle(AIS_InteractiveObject) m_parentObject{};
    TopoDS_Shape m_selectedShape{};

    //gp_Pnt m_centerPoint;

    bool operator==(const SelectedEntity &other) const
    {
        return m_selectedShape.IsSame(other.m_selectedShape);
    }
};
} // namespace View
