# Qt_OCC CAD Application

Qt_OCC is a desktop CAD application developed using the power of the Qt framework for the user interface and OpenCASCADE Technology (OCCT) as the geometry kernel. It aims to provide a platform for 3D modeling and visualization, incorporating a range of features for shape creation, manipulation, and analysis.

## ✨ Features

### 📂 File Management
*   **Input/Output**: Support for creating new projects, opening existing files, saving, and exporting models.

### 🎥 Visualization & Navigation
*   **Standard Views**: Quickly switch between Isometric, Top, Bottom, Left, Right, Front, and Back views.
*   **Display Modes**: Toggle between Wireframe, Shaded, and Shaded with Edges rendering styles.
*   **View Control**: Auto-fit view to scene content.

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

## 🤝 Contributing

Contributions are welcome! If you have a suggestion or want to fix a bug, please feel free to open an issue or submit a pull request.

When contributing code, please try to adhere to the existing coding style.
