# 3D PDF Export Feature

This application includes a feature to export 3D CAD models directly to **3D PDF** format. 

## Internal Workflow

Currently, the export dialogue offers two conceptual methods for generating the 3D PDF.

### Method 1: OCCT -> STEP/STL -> U3D -> 3D PDF (Current Default)
This is the active implementation that provides a robust, mesh-based 3D PDF export:
1. **Model Extraction**: The current TopoDS shapes are gathered from the OCAF document and unified into a single `TopoDS_Compound`.
2. **Mesh Generation**: OpenCASCADE's `BRepMesh` converts the exact BRep geometry into a triangulated mesh structure.
3. **STL Export**: The mesh is exported to an intermediate binary STL file (`temp.stl`) via `CoreApi::ExportApi`.
4. **U3D Conversion**: The application invokes a Python script using the `pymeshlab` library (a modern replacement for `meshlabserver`) to convert the STL mesh into the U3D (Universal 3D) format.
5. **PDF Compilation**: A temporary LaTeX document (`.tex`) is generated, utilizing the `media9` package to embed the U3D file. Finally, `pdflatex` is executed in the background to compile the document into the final 3D PDF.

### Method 2: OCCT -> OSG -> libPRC -> 3D PDF (Future Scope)
This method is currently disabled in the UI but represents a future planned enhancement for higher fidelity exports:
- Instead of using a lossy triangulated mesh (U3D), this pipeline intends to use the **PRC (Product Representation Compact)** format.
- PRC allows for the embedding of precise BRep (Boundary Representation) data, meaning curved surfaces and exact geometry are preserved mathematically rather than being tessellated.
- The pipeline will theoretically involve translating OCCT shapes into an OSG (OpenSceneGraph) tree and subsequently using `libPRC` to compile the final PDF.

## Requirements
To successfully use Method 1 on your local machine, ensure you have the following installed:
* Python 3.x with the `pymeshlab` package (`pip install pymeshlab`)
* MiKTeX or TeX Live (with `pdflatex` available in your system's PATH)
