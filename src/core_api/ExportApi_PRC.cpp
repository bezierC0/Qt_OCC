#ifdef USE_LIBPRC

#include "ExportApi.h"    // contains the ExportApi class declaration with ExportToPrcPdf
#include <oPRCFile.h>     // asymptote PRC writer
#include <hpdf.h>         // libharu PDF library (core)
#include <hpdf_u3d.h>     // libharu 3D view / annotation API (HPDF_Create3DView etc.)

// -- OCCT headers ----------------------------------------------------------
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <Poly_Triangulation.hxx>
#include <gp_Pnt.hxx>

// -- STL -------------------------------------------------------------------
#include <fstream>
#include <cmath>
#include <vector>
#include <string>
#include <cstdlib>    // std::system

namespace CoreApi {
static bool writePrcFile(const TopoDS_Compound& compound,
                         const std::string&     prcPath,
                         std::string&           errMsg)
{
    // Tessellate the shape
    BRepMesh_IncrementalMesh mesh(compound, 0.1, /*isRelative=*/false, 0.5);
    mesh.Perform();

    // Open the PRC output stream
    std::ofstream out(prcPath, std::ios::binary);
    if (!out.is_open()) {
        errMsg = "Cannot open PRC output file: " + prcPath;
        return false;
    }

    oPRCFile prcFile(out);
    prcFile.begingroup("model");

    // PRCmaterial: (ambient, diffuse, emissive, specular, alpha, shininess)
    PRCmaterial mat(
        RGBAColour(0.2, 0.2, 0.2, 1.0),   // ambient
        RGBAColour(0.6, 0.65, 0.8, 1.0),   // diffuse (slight blue-grey)
        RGBAColour(0.0, 0.0, 0.0, 1.0),   // emissive
        RGBAColour(0.8, 0.8, 0.8, 1.0),   // specular
        1.0,                               // alpha (fully opaque)
        32.0                               // shininess
    );

    // Iterate over all faces
    for (TopExp_Explorer ex(compound, TopAbs_FACE); ex.More(); ex.Next())
    {
        const TopoDS_Face&         face = TopoDS::Face(ex.Current());
        TopLoc_Location            loc;
        Handle(Poly_Triangulation) tri  = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull() || tri->NbTriangles() == 0) continue;

        const bool reversed = (face.Orientation() == TopAbs_REVERSED);
        const int  nbNodes  = tri->NbNodes();
        const int  nbTris   = tri->NbTriangles();

        // Build vertex array: double[nbNodes][3]
        std::vector<std::array<double,3>> P(nbNodes);
        for (int i = 1; i <= nbNodes; ++i) {
            gp_Pnt p = tri->Node(i).Transformed(loc);
            P[i-1] = { p.X(), p.Y(), p.Z() };
        }

        // Build triangle index array: uint32_t[nbTris][3]
        std::vector<std::array<uint32_t,3>> PI(nbTris);
        for (int i = 1; i <= nbTris; ++i) {
            int n1, n2, n3;
            tri->Triangle(i).Get(n1, n2, n3);
            if (reversed) std::swap(n2, n3);
            PI[i-1] = { static_cast<uint32_t>(n1-1),
                        static_cast<uint32_t>(n2-1),
                        static_cast<uint32_t>(n3-1) };
        }

        // addTriangles(nP, P, nI, PI, material,
        //              nN, N, NI,   <- normals (NULL = skip)
        //              nT, T, TI,   <- texcoords (NULL)
        //              nC, C, CI,   <- per-vertex colors (NULL)
        //              nM, M, MI,   <- per-face materials (NULL)
        //              ca)          <- overall opacity (1.0 = opaque)
        prcFile.addTriangles(
            static_cast<uint32_t>(nbNodes),
            reinterpret_cast<const double(*)[3]>(P.data()),
            static_cast<uint32_t>(nbTris),
            reinterpret_cast<const uint32_t(*)[3]>(PI.data()),
            mat,
            0, nullptr, nullptr,   // normals
            0, nullptr, nullptr,   // texture coords
            0, nullptr, nullptr,   // per-vertex colors
            0, nullptr, nullptr,   // per-face materials
            1.0);                  // opacity
    }

    prcFile.endgroup();
    prcFile.finish();
    out.close();
    return true;
}

// ---------------------------------------------------------------------------
// Step 2: Embed .prc into a PDF using libHaru
// ---------------------------------------------------------------------------
static void hpdfErrorHandler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void* /*user_data*/)
{
    // libharu calls this on error; convert it to an exception for clean stack unwinding.
    throw std::runtime_error(
        "libharu error: 0x" + std::to_string(static_cast<unsigned>(error_no)) +
        "  detail: "        + std::to_string(static_cast<unsigned>(detail_no)));
}

static bool embedPrcAsPdf(
    const std::string&    prcPath,
    const std::string&    pdfPath,
    const Quantity_Color& bgColor,
    const Bnd_Box&        bbox,
    std::string&          errMsg)
{
    // Compute bounding-box centre and orbit radius for the default 3D view
    Standard_Real xmin=0, ymin=0, zmin=0, xmax=0, ymax=0, zmax=0;
    if (!bbox.IsVoid())
        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    const HPDF_REAL cx  = static_cast<HPDF_REAL>((xmin + xmax) / 2.0);
    const HPDF_REAL cy  = static_cast<HPDF_REAL>((ymin + ymax) / 2.0);
    const HPDF_REAL cz  = static_cast<HPDF_REAL>((zmin + zmax) / 2.0);
    const HPDF_REAL dx  = static_cast<HPDF_REAL>(xmax - xmin);
    const HPDF_REAL dy  = static_cast<HPDF_REAL>(ymax - ymin);
    const HPDF_REAL dz  = static_cast<HPDF_REAL>(zmax - zmin);
    HPDF_REAL roo = static_cast<HPDF_REAL>(std::sqrt(dx*dx + dy*dy + dz*dz));
    if (roo < 1e-3f) roo = 100.0f;

    HPDF_Doc pdf = HPDF_New(hpdfErrorHandler, nullptr);
    if (!pdf) {
        errMsg = "HPDF_New failed - cannot create PDF document.";
        return false;
    }

    try
    {
        // Load the PRC file as a U3D stream (libharu uses the same API for both)
        HPDF_U3D u3d = HPDF_LoadU3DFromFile(pdf, prcPath.c_str());
        if (!u3d)
            throw std::runtime_error("HPDF_LoadU3DFromFile failed: " + prcPath);

        // Create a default 3D view with camera + background colour
        HPDF_Dict view = HPDF_Create3DView(pdf->mmgr, "Default");
        if (!view)
            throw std::runtime_error("HPDF_Create3DView failed.");

        // Camera: position at (cx, cy, cz + roo), looking at (cx, cy, cz), up = Z
        HPDF_3DView_SetCamera(view,
            cx, cy, cz,          // centre of orbit (coo)
            0, 0, 1,             // c2c vector (camera-to-centre direction)
            roo,                 // radius of orbit
            0.0f);               // roll angle

        HPDF_3DView_SetPerspectiveProjection(view, 30.0f); // 30-degree FOV

        HPDF_3DView_SetBackgroundColor(
            view,
            static_cast<HPDF_REAL>(bgColor.Red()),
            static_cast<HPDF_REAL>(bgColor.Green()),
            static_cast<HPDF_REAL>(bgColor.Blue()));

        HPDF_3DView_SetLighting(view, "White");

        HPDF_U3D_Add3DView(u3d, view);
        HPDF_U3D_SetDefault3DView(u3d, "Default");

        // Create an A4 page and place the 3D annotation (80 % of page width)
        HPDF_Page page = HPDF_AddPage(pdf);
        HPDF_Page_SetWidth (page, 595.0f);  // A4 pts
        HPDF_Page_SetHeight(page, 842.0f);

        HPDF_Rect rect = { 50.0f, 100.0f, 545.0f, 792.0f };
        HPDF_Page_Create3DAnnot(page, rect, u3d);

        // Save to file
        if (HPDF_SaveToFile(pdf, pdfPath.c_str()) != HPDF_OK)
            throw std::runtime_error("HPDF_SaveToFile failed: " + pdfPath);
    }
    catch (const std::exception& e)
    {
        errMsg = e.what();
        HPDF_Free(pdf);
        return false;
    }

    HPDF_Free(pdf);
    return true;
}
// ---------------------------------------------------------------------------
// Public API  (static method on CoreApi::ExportApi)
// ---------------------------------------------------------------------------
bool ExportApi::ExportToPrcPdf(
    const TopoDS_Compound& compound,
    const std::string&     pdfPath,
    const Quantity_Color&  bgColor,
    std::string&           errMsg)
{
    // Derive PRC path alongside the final PDF
    const std::string pdfDir  = pdfPath.substr(0, pdfPath.rfind('\\') + 1);
    const std::string prcPath = pdfDir + "model_prc.prc";

    // Step 1: Write .prc via oPRCFile (asymptote)
    if (!writePrcFile(compound, prcPath, errMsg))
        return false;

    // Step 2: Compute bounding box for camera hints
    Bnd_Box bbox;
    BRepBndLib::Add(compound, bbox);

    // Step 3: Embed PRC into PDF using libHaru
    return embedPrcAsPdf(prcPath, pdfPath, bgColor, bbox, errMsg);
}

} // namespace CoreApi

#endif // USE_LIBPRC
