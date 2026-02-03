#include "OCCView.h"
#include <Graphic3d_ClipPlane.hxx>

namespace View
{
    ClippingPlane::ClippingPlane() = default;
    
    ClippingPlane::ClippingPlane(const Handle(Graphic3d_ClipPlane)& clipPlane, const bool isOn, const bool isCapping)
        : m_clipPlane(clipPlane)
    {
        m_clipPlane->SetOn(isOn);
        m_clipPlane->SetCapping(isCapping);
    }
} // namespace View
