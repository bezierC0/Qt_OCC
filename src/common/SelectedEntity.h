#pragma once
#include <Standard_Handle.hxx>
#include <Standard_WarningsDisable.hxx>
#include <Standard_WarningsRestore.hxx>

#include <AIS_ViewController.hxx>
#include <TDF_Label.hxx> // Needed for TDF_Label member

// Forward declarations
class AIS_InteractiveObject;
class AIS_Shape;
class TopoDS_Shape;

namespace View
{
class SelectedEntity {
public:
    SelectedEntity(const Handle(AIS_InteractiveObject)& parentObject, const Handle(AIS_Shape)& selectedShape, const TDF_Label& label);

    Handle(AIS_InteractiveObject) GetParentInteractiveObject() const;
    Handle(AIS_Shape) GetSelectedShape() const;
    const TDF_Label& GetLabel() const;
    
    Handle(AIS_InteractiveObject) m_parentObject;
    Handle(AIS_Shape) m_selectedShape;
    TDF_Label m_label;

    bool operator==(const SelectedEntity &other) const;
};
} // namespace View
