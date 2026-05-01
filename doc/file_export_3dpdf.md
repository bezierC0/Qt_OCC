# 3D PDF Export Feature

This application includes a feature to export 3D CAD models directly to **3D PDF** format.

## Internal Workflow

The export dialogue offers two methods for generating the 3D PDF, selectable at runtime.
Method 2 is only available when the project is compiled with the CMake option `EXPORT_3DPDF_USE_LIBPRC=ON`.

---

### Method 1: OCCT -> STL -> U3D -> 3D PDF (Default)

This is the default implementation, using a pipeline of freely available tools:

```mermaid
flowchart LR
    A[OCCT Shapes] --> B[BRepMesh STL]
    B --> C[pymeshlab U3D]
    C --> D[LaTeX media9]
    D --> E[pdflatex]
    E --> F[3D PDF]
```

1. **Model Extraction**: All visible TopoDS shapes are gathered from the OCAF document and merged into a single `TopoDS_Compound`.
2. **STL Export**: `CoreApi::ExportApi::ExportToStl()` calls `BRepMesh_IncrementalMesh` to tessellate the geometry and writes it to a temporary STL file.
3. **U3D Conversion**: A Python subprocess runs `pymeshlab` to load the STL and re-save it as a U3D (Universal 3D) file. This replaces the deprecated `meshlabserver`.
4. **LaTeX / PDF Compilation**: A `.tex` file is generated using the `media9` package to embed the U3D stream, including camera hints (`3Dcoo`, `3Droo`) computed from the model bounding box and the viewport background colour (`3Dbg`). `pdflatex` (MiKTeX) is then invoked to produce the final PDF.

**Key files**: `DialogExport3DPdf.cpp` -> `CoreApi::ExportApi::ExportToStl()`

---

### Method 2: OCCT -> PRC -> 3D PDF (libPRC + libHaru)

This method produces higher-quality 3D PDFs with full triangulated geometry embedded in the compact **PRC (Product Representation Compact)** format, read natively by Adobe Acrobat. It does **not** require Python, pymeshlab, or pdflatex.

```mermaid
flowchart LR
    A[OCCT Shapes] --> B[BRepMesh Tessellation]
    B --> C[oPRCFile asymptote.lib]
    C --> D[.prc file]
    D --> E[libHaru HPDF_LoadU3DFromFile]
    E --> F[3D PDF]
```

#### Pipeline

1. **Model Extraction**: Same as Method 1 -- a `TopoDS_Compound` is built from all visible shapes.
2. **BRepMesh Tessellation**: `BRepMesh_IncrementalMesh` is called directly in C++ (linear deflection 0.1, angular 0.5 rad) to produce face-level `Poly_Triangulation` data.
3. **PRC File Generation** (`ExportApi_PRC.cpp`):
   - For each `TopoDS_Face`, vertex coordinates and triangle index arrays are extracted and passed to `oPRCFile::addTriangles()` -- the core writer from the Asymptote project (`asymptote.lib`).
   - A default grey-blue `PRCmaterial` (ambient / diffuse / specular / shininess) is applied to all faces.
   - The PRC binary stream is written to a temporary `.prc` file via `std::ofstream`.
4. **PDF Embedding** (`ExportApi_PRC.cpp`):
   - `HPDF_LoadU3DFromFile()` loads the `.prc` file into a libharu document. (libharu's U3D/PRC API is unified.)
   - `HPDF_Create3DView()` + `HPDF_3DView_SetCamera()` set the camera position using the bounding-box centre and orbit radius.
   - `HPDF_3DView_SetBackgroundColor()` applies the current viewport background colour.
   - `HPDF_3DView_SetPerspectiveProjection()` sets a 30-degree field of view.
   - `HPDF_Page_Create3DAnnot()` places the 3D annotation on an A4 page.
   - `HPDF_SaveToFile()` writes the complete PDF.

#### CMake Build Flags

| Option | Default | Description |
|--------|---------|-------------|
| `EXPORT_3DPDF_USE_LIBPRC` | `OFF` | Enable Method 2 compilation |
| `LIBPRC_DIR` | `XXX/libPRC` | Root of the libPRC package |
| `LIBPNG_DIR` | `XXX/libpng` | Root of libpng (libPRC dependency) |
| `LIBHARU_DIR` | `XXX/libharu-2.3.0` | Root of libharu |

When `EXPORT_3DPDF_USE_LIBPRC=ON`, the preprocessor macro `USE_LIBPRC` is defined project-wide.
The UI radio button for Method 2 is enabled at compile time via `#ifdef USE_LIBPRC`.

#### Key Files

| File | Role |
|------|------|
| `src/core_api/ExportApi.h` | Declares `ExportApi::ExportToPrcPdf()` inside `#ifdef USE_LIBPRC` |
| `src/core_api/ExportApi_PRC.cpp` | Full implementation: tessellation, PRC write, libHaru PDF embedding |
| `src/ui/DialogExport3DPdf.cpp` | UI routing -- calls `ExportApi::ExportToPrcPdf()` when Method 2 is selected |

---

## Requirements

### Method 1
- Python 3.x with the `pymeshlab` package (`pip install pymeshlab`)
- MiKTeX or TeX Live (`pdflatex` available in PATH)

### Method 2 (compile-time dependencies)
- `XXX\libPRC` -- libPRC package (includes asymptote.lib PRC writer)
- `XXX\libpng` -- libpng (libPRC dependency for image encoding)
- `XXX\libharu-2.3.0` -- libharu PDF library (libhpdf.lib)
- CMake configured with `-DEXPORT_3DPDF_USE_LIBPRC=ON`
