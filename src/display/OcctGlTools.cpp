#include "OcctGlTools.h"

#include <OpenGl_View.hxx>
#include <OpenGl_Window.hxx>

Handle(OpenGl_Context) OcctGlTools::GetGlContext(const Handle(V3d_View)& theView)
{
    Handle(OpenGl_View) aGlView = Handle(OpenGl_View)::DownCast(theView->View());
    return aGlView->GlWindow()->GetGlContext();
}
