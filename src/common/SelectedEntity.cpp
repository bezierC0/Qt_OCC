#include "SelectedEntity.h"
#include <AIS_InteractiveObject.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Shape.hxx>

namespace View {

SelectedEntity::SelectedEntity(const Handle(AIS_InteractiveObject)& parentObject, const Handle(AIS_Shape)& selectedShape, const TDF_Label& label)
    : m_parentObject(parentObject), m_selectedShape(selectedShape), m_label(label)
{
}

Handle(AIS_InteractiveObject) SelectedEntity::GetParentInteractiveObject() const {
    return m_parentObject;
}

Handle(AIS_Shape) SelectedEntity::GetSelectedShape() const {
    return m_selectedShape;
}

const TDF_Label& SelectedEntity::GetLabel() const {
    return m_label;
}

bool SelectedEntity::operator==(const SelectedEntity &other) const
{
    // Compare labels
    if (!m_label.IsEqual(other.m_label)) {
        return false;
    }
    
    // Compare shapes (if both exist)
    if (!m_selectedShape.IsNull() && !other.m_selectedShape.IsNull()) {
        return m_selectedShape->Shape().IsSame(other.m_selectedShape->Shape());
    }
    
    return m_selectedShape.IsNull() == other.m_selectedShape.IsNull();
}

} // namespace View
