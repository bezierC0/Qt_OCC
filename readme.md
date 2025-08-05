# Qt_OCC CAD Application

Qt_OCC is a desktop CAD application developed using the power of the Qt framework for the user interface and OpenCASCADE Technology (OCCT) as the geometry kernel. It aims to provide a platform for 3D modeling and visualization, incorporating a range of features for shape creation, manipulation, and analysis.

## ✨ Features

*   **3D Model Visualization**: Import, view, and interact with 3D models.
*   **Shape Creation**: Tools for creating basic 2D and 3D geometric shapes.
*   **Boolean Operations**: Perform Union, Intersection, and Difference operations on solids.
*   **Transformations**: Translate, rotate, and scale objects using an interactive 3D manipulator.
*   **Assembly Tools**: Explode assemblies for better visualization of components.
*   **Patterning**: Create linear and circular patterns from existing shapes.
*   **Advanced Selection**: Use selection filters to precisely select vertices, edges, faces, or solids.
*   **Display Modes**: Switch between different display modes like shaded, wireframe, and shaded with edges.
*   **Clipping Planes**: Analyze models with dynamic section views using clipping planes.
*   **Work Plane System**: Define and manipulate a custom work plane for easier sketching and modeling.
*   **Analysis Tools**: Includes features like interference detection.

## 🛠️ Technology Stack

*   **C++**: The core application logic is written in modern C++.
*   **Qt Framework**: Used for the entire graphical user interface (GUI), including the ribbon bar, model tree, and 3D view integration.
*   **OpenCASCADE Technology (OCCT)**: The powerful open-source 3D geometric modeling kernel that handles all CAD operations.
*   **CMake**: The cross-platform build system used to configure and build the project.

## 🚀 Getting Started

Follow these instructions to get a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

You will need to have the following software installed on your system:

*   **A C++17 compliant compiler**:
    *   Windows: MSVC 2019 or newer
*   **CMake**: Version 3.16 or higher.
*   **Qt Framework**: Version 6.2 or higher. Make sure to install the components that match your compiler (e.g., MSVC 2019 64-bit).
*   **OpenCASCADE Technology (OCCT)**: Version 7.6.0 or higher. You will need to build or install it and make sure it's findable by CMake.

### Compilation Steps

1.  **Clone the repository**

    ```bash
    git clone https://github.com/bezierC0/Qt_OCC.git
    cd Qt_OCC
    ```

2.  **Create a build directory**

    It's best practice to perform an out-of-source build.

    ```bash
    mkdir build
    cd build
    ```

3.  **Configure with CMake**

    You will need to tell CMake where to find your Qt and OCCT installations.

    ```bash
    # Example on Windows with Visual Studio
    cmake .. -G "Visual Studio 17 2019" -A x64 -DQt6_DIR=C:/Qt/6.5.0/msvc2019_64/lib/cmake/Qt6 -DOpenCASCADE_DIR=C:/Libs/OCCT/occt-7.8.0-vc14-64 -DOpenCASCADE_3RDPARTY_DIR=C:/Libs/OCCT/3rdparty-vc14-64

    # Example on Linux
    # cmake .. -DCMAKE_PREFIX_PATH="/path/to/qt;/path/to/occt"
    ```
    *Replace the paths to `Qt5_DIR` and `OCCT_DIR` with the actual paths on your system.*

4.  **Build the project**

    ```bash
    cmake --build . --config Release
    ```

5.  **Run the application**

    The executable will be located in the `build/bin/Release` (on Windows) or `build/bin` (on Linux) directory.

## 💻 Usage

After launching the application, you will be presented with the main window.

*   **Ribbon Toolbar**: The top of the window features a ribbon-style toolbar containing all the tools for file operations, viewing, modeling, and analysis.
*   **Model Tree**: The panel on the left displays a tree view of all the shapes and objects in the current document. You can select, show, or hide objects from this tree.
*   **3D View**: The central area is the interactive 3D viewport.
    *   **Rotate**: Left-click and drag.
    *   **Pan**: Middle-click and drag.
    *   **Zoom**: Use the mouse scroll wheel.

## 🤝 Contributing

Contributions are welcome! If you have a suggestion or want to fix a bug, please feel free to open an issue or submit a pull request.

When contributing code, please try to adhere to the existing coding style, which is based on the LLVM style guide. The configuration can be found in the `.clang-format` file in the root of the repository.

---
