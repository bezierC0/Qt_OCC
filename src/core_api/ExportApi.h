#pragma once

#include "CoreApiGlobal.h"
#include <TopoDS_Shape.hxx>
#include <TopoDS_Compound.hxx>
#include <Quantity_Color.hxx>
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

#ifdef USE_LIBPRC
    /**
     * @brief Export a TopoDS_Compound to a 3D PDF using the libPRC pipeline.
     *
     * Tessellates the OCCT shape, writes a .prc file via oPRCFile (asymptote),
     * then embeds the PRC into a PDF using LaTeX / media9.
     *
     * @param compound  The OCCT compound to export.
     * @param pdfPath   Absolute output path for the resulting 3D PDF.
     * @param bgColor   Background colour to embed in the PDF 3D view.
     * @param errMsg    Populated with an error description on failure.
     * @return true on success, false on failure.
     */
    static bool ExportToPrcPdf(
        const TopoDS_Compound& compound,
        const std::string&     pdfPath,
        const Quantity_Color&  bgColor,
        std::string&           errMsg);
#endif // USE_LIBPRC
};

} // namespace CoreApi
