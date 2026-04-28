#include "ExportApi.h"

#include <BRepMesh_IncrementalMesh.hxx>
#include <StlAPI_Writer.hxx>
#include <Message_ProgressRange.hxx>
#include <TopoDS_Compound.hxx>

namespace CoreApi {

bool ExportApi::ExportToStl(const TopoDS_Shape& shape, const std::string& filename, double deflection, double angle)
{
    if (shape.IsNull()) {
        return false;
    }

    try {
        // 1. Mesh the shape
        BRepMesh_IncrementalMesh mesher(shape, deflection, false, angle, true);
        mesher.Perform();
        
        if (!mesher.IsDone()) {
            return false;
        }

        // 2. Export to STL
        StlAPI_Writer stlWriter;
        stlWriter.ASCIIMode() = false; // Binary mode
        return stlWriter.Write(shape, filename.c_str());
    }
    catch (...) {
        // Handle any OCC exceptions during meshing or writing
        return false;
    }
}

} // namespace CoreApi
