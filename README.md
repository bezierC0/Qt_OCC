# Qt_OCC CAD Application

Qt_OCC is a desktop CAD and point-cloud application developed using the Qt framework for the user interface and OpenCASCADE Technology (OCCT) as the geometry and visualization kernel. It provides tools for 3D modeling, point-cloud visualization, manipulation, measurement, and analysis.

## ✨ Features

### 📂 File Management
*   **CAD Input/Output**: Import STEP/IGES models and export supported CAD formats.
*   **Point Cloud Import**: Import `.xyz`, `.txt`, `.pts`, and `.ply` point-cloud files.

### 🎥 Visualization & Navigation
*   **Standard Views**: Quickly switch between Isometric, Top, Bottom, Left, Right, Front, and Back views.
*   **Display Modes**: Toggle between Wireframe, Shaded, and Shaded with Edges rendering styles.
*   **View Control**: Auto-fit view to scene content.

### ☁️ Point Cloud
*   **Supported Data**: XYZ and XYZRGB records, PTS files with an optional point-count header, ASCII PLY, and Binary Little Endian PLY.
*   **Native OCCT Display**: Render point data with `AIS_PointCloud` without creating a topological vertex for every point.
*   **Display Controls**: Show or hide the cloud, fit it to the view, remove it, adjust point size, and reset display settings.
*   **Color Modes**: Use original RGB values, a selected single color, or colors generated from Z height.
*   **Cloud Information**: Inspect source and displayed point counts, invalid records, RGB availability, file size, bounds, and dimensions.
*   **Large File Protection**: Parse files in a worker thread with progress and cancellation. Limit displayed points to 500K, 1M, 2M, 5M, or all points, using uniform sampling when required.
*   **CAD/CAE Isolation**: Keep point clouds out of topology-dependent modeling, measurement, export, interference, and CAE operations.

### 🛠️ Modeling Tools
*   **2D Primitives**: Create Points, Lines, Rectangles, Circles, Arcs, Ellipses, Polygons, Bezier Curves, and NURBS Curves.
*   **3D Primitives**: Generate Boxes, Spheres, Cylinders, Cones, and Pyramids.
*   **Boolean Operations**: Perform Union (Fuse), Intersection (Common), and Difference (Cut) on solid bodies.
*   **Modifications**:
    *   **Mirror**: Mirror objects by Plane or Axis.
    *   **Pattern**: Create Linear and Circular patterns.
    *   **Shell**: Convert solids into shell structures.

### 📏 Measurement & Analysis
*   **Measurements**: Precise tools to measure Distance, Edge Length, Arc Length/Radius, and Angles.
*   **Analysis**: Check for Interference between objects.
*   **Assembly Visualization**: Explode assemblies to inspect internal components.
*   **Clipping**: Apply clipping planes for cross-sectional views.

### ⚙️ Utilities & Customization
*   **Transform**: Interactive tools to move, rotate, and scale objects.
*   **Work Plane**: Configurable coordinate system helper.
*   **Selection Filters**: Filter selection by vertex, edge, face, or solid.
*   **Theme Support**: Switch between Light and Dark themes.
*   **Localization**: Multi-language support.

## 🛠️ Technology Stack

*   **C++**: The core application logic is written in modern C++ (C++17 standard).
*   **Qt Framework**: Used for the entire graphical user interface (GUI), utilizing SARibbon for a modern ribbon-style toolbar.
*   **OpenCASCADE Technology (OCCT)**: The powerful open-source 3D geometric modeling kernel that handles all CAD operations.
*   **CMake**: The cross-platform build system used to configure and build the project.

## 📚 Documentation

*   **Development and refactoring notes**: [`doc/development_refactoring.md`](doc/development_refactoring.md)
*   **3D Busbar Routing feature**: [`doc/tool_busbar.md`](doc/tool_busbar.md)
*   **3D PDF export feature**: [`doc/file_export_3dpdf.md`](doc/file_export_3dpdf.md)
*   **Animation feature**: [`doc/animation.md`](doc/animation.md)
*   **Point Cloud development and manual test plan**: [`doc/point_cloud_development_plan.md`](doc/point_cloud_development_plan.md)
*   **Point Cloud sample data**: [`samples/pointcloud/README.md`](samples/pointcloud/README.md)
*   **Localized READMEs**: [`doc/CN/README_CN.md`](doc/CN/README_CN.md), [`doc/JP/README_JP.md`](doc/JP/README_JP.md), [`doc/FR/README_FR.md`](doc/FR/README_FR.md), [`doc/RU/README_RU.md`](doc/RU/README_RU.md), [`doc/ES/README_ES.md`](doc/ES/README_ES.md)

## 🚀 Getting Started

Follow these instructions to get a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

You will need to have the following software installed on your system:

*   **A C++17 compliant compiler**:
    *   Windows: MSVC 2019 or newer
*   **CMake**: Version 3.16 or higher.
*   **Qt Framework**: Version 6.2 or higher. Make sure to install the components that match your compiler (e.g., MSVC 2019 64-bit).
*   **OpenCASCADE Technology (OCCT)**: Version 7.6.0 or higher.

### Compilation Steps

1.  **Clone the repository**

    ```bash
    git clone https://github.com/bezierC0/Qt_OCC.git
    cd Qt_OCC
    ```

2.  **Create a build directory**

    ```bash
    mkdir build
    cd build
    ```

3.  **Configure with CMake**

    Ensure you set the correct paths to your Qt and OCCT installations.

    ```bash
    # Example on Windows with Visual Studio
    cmake .. -G "Visual Studio 17 2019" -A x64 \
    -DQt5_DIR="C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5" \
    -DOpenCASCADE_DIR="C:/Libs/OCCT/occt-7.8.0-vc14-64" \
    -DOpenCASCADE_3RDPARTY_DIR="C:/Libs/OCCT/3rdparty-vc14-64"
    ```

4.  **Build the project**

    ```bash
    cmake --build . --config Release
    ```

5.  **Run the application**

    The executable will be located in the `build/bin/Release` (on Windows) or `build/bin` (on Linux) directory.

### Importing a Point Cloud

1. Open **File > Import**.
2. Select an `.xyz`, `.txt`, `.pts`, or `.ply` file.
3. Use the **Point Cloud** panel to inspect statistics and change the display settings.
4. Use **Fit Point Cloud** to frame the imported data.

Importing another model or point cloud replaces the current scene. Binary Big Endian PLY, PCD, LAS/LAZ, registration, filtering, and surface reconstruction are not currently supported. CAD- and CAE-specific commands are unavailable while a point cloud is loaded.

Manual test files are available in [`samples/pointcloud`](samples/pointcloud). The compact `binary_colored_cube.ply` sample exercises Binary Little Endian PLY with mixed property order and RGB data.

## 🤝 Contributing

Contributions are welcome! If you have a suggestion or want to fix a bug, please feel free to open an issue or submit a pull request.

When contributing code, please try to adhere to the existing coding style.
