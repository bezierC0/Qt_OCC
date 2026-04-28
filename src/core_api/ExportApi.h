#pragma once

#include "CoreApiGlobal.h"
#include <TopoDS_Shape.hxx>
#include <string>

namespace CoreApi {

/**
 * @brief API for handling exports of OpenCASCADE shapes.
 */
class CORE_API_EXPORT ExportApi
{
public:
    /**
     * @brief Exports a TopoDS_Shape to an STL file.
     * 
     * @param shape The shape to be exported.
     * @param filename The destination STL file path.
     * @param deflection The linear deflection for meshing (default: 0.1).
     * @param angle The angular deflection for meshing (default: 0.5 rad).
     * @return true if export is successful, false otherwise.
     */
    static bool ExportToStl(const TopoDS_Shape& shape, const std::string& filename, double deflection = 0.1, double angle = 0.5);
};

} // namespace CoreApi
