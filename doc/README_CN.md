# Qt_OCC CAD 应用程序

Qt_OCC 是一个使用 Qt 框架开发用户界面并以 OpenCASCADE Technology (OCCT) 作为几何内核的桌面 CAD 应用程序。它旨在提供一个 3D 建模和可视化的平台，集成了形状创建、操作和分析的一系列功能。

## ✨ 功能特性

### 📂 文件管理
*   **输入/输出**: 支持新建项目、打开现有文件、保存以及导出模型。

### 🎥 可视化与导航
*   **标准视图**: 快速切换等轴测(Isometric)、顶、底、左、右、前、后视图。
*   **显示模式**: 支持线框(Wireframe)、着色(Shaded)以及带边框着色(Shaded with Edges)模式。
*   **视图控制**: 自动适应视图以显示所有内容。

### 🛠️ 建模工具
*   **2D 图元**: 创建点、直线、矩形、圆、圆弧、椭圆、多边形、贝塞尔曲线 (Bezier Curve) 和 NURBS 曲线。
*   **3D 图元**: 生成立方体、球体、圆柱体、圆锥体和棱锥体。
*   **布尔运算**: 对实体进行并集 (Fuse)、交集 (Common) 和差集 (Cut) 运算。
*   **修改操作**:
    *   **镜像**: 通过平面或轴镜像对象。
    *   **阵列**: 创建线性或圆形阵列。
    *   **抽壳 (Shell)**: 将实体转换为抽壳结构。

### 📏 测量与分析
*   **测量**: 提供距离、边长、弧长/半径和角度的精确测量工具。
*   **分析**: 检查对象之间的干涉情况。
*   **装配可视化**: 爆炸视图以检查内部组件。
*   **剖切**: 应用剖切平面查看截面。

### ⚙️ 工具与自定义
*   **变换**: 交互式工具用于移动、旋转和缩放对象。
*   **工作平面**: 可配置的坐标系辅助工具。
*   **选择过滤器**: 支持点、边、面或实体的精确选择。
*   **主题支持**: 支持切换亮色和暗色主题。
*   **本地化**: 多语言支持。

## 🛠️ 技术栈

*   **C++**: 核心逻辑采用现代 C++ (C++17 标准) 编写。
*   **Qt Framework**: 用于整个图形用户界面 (GUI)，使用 SARibbon 实现现代化的 Ribbon 风格工具栏。
*   **OpenCASCADE Technology (OCCT)**: 强大的开源 3D 几何建模内核，处理所有 CAD 操作。
*   **CMake**: 用于配置和构建项目的跨平台构建系统。

## 🚀 快速开始

按照以下说明在本地机器上设置项目，以便进行开发和测试。

### 先决条件

您的系统需要安装以下软件：

*   **符合 C++17 标准的编译器**:
    *   Windows: MSVC 2019 或更新版本
*   **CMake**: 3.16 或更高版本。
*   **Qt Framework**: 6.2 或更高版本。请确保安装与您的编译器匹配的组件。
*   **OpenCASCADE Technology (OCCT)**: 7.6.0 或更高版本。

### 编译步骤

1.  **克隆仓库**

    ```bash
    git clone https://github.com/bezierC0/Qt_OCC.git
    cd Qt_OCC
    ```

2.  **创建构建目录**

    ```bash
    mkdir build
    cd build
    ```

3.  **配置 CMake**

    确保设置了 Qt 和 OCCT 安装的正确路径。

    ```bash
    # Windows Visual Studio 示例
    cmake .. -G "Visual Studio 17 2019" -A x64 \
    -DQt5_DIR="C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5" \
    -DOpenCASCADE_DIR="C:/Libs/OCCT/occt-7.8.0-vc14-64" \
    -DOpenCASCADE_3RDPARTY_DIR="C:/Libs/OCCT/3rdparty-vc14-64"
    ```

4.  **构建项目**

    ```bash
    cmake --build . --config Release
    ```

5.  **运行应用程序**

    可执行文件将位于 `build/bin/Release` (Windows) 或 `build/bin` (Linux) 目录中。

## 🤝 贡献

欢迎贡献代码！如果您有建议或想要修复错误，请随时提交 Issue 或 Pull Request。
