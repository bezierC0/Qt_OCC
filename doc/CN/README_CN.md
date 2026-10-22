# Qt_OCC CAD 应用程序

Qt_OCC 是一个使用 Qt 框架开发用户界面并以 OpenCASCADE Technology (OCCT) 作为几何与可视化内核的桌面 CAD 和点云应用程序。它提供 3D 建模、点云显示、操作、测量和分析功能。

## ✨ 功能特性

### 📂 文件管理
*   **CAD 输入/输出**: 导入 STEP/IGES 模型，并导出支持的 CAD 格式。
*   **点云导入**: 导入 `.xyz`、`.txt`、`.pts` 和 `.ply` 点云文件。

### 🎥 可视化与导航
*   **标准视图**: 快速切换等轴测(Isometric)、顶、底、左、右、前、后视图。
*   **显示模式**: 支持线框(Wireframe)、着色(Shaded)以及带边框着色(Shaded with Edges)模式。
*   **视图控制**: 自动适应视图以显示所有内容。

### ☁️ 点云
*   **支持的数据**: XYZ、XYZRGB、带可选点数头的 PTS、ASCII PLY 和 Binary Little Endian PLY。
*   **OCCT 原生显示**: 使用 `AIS_PointCloud` 显示点数据，不为每个点创建拓扑顶点。
*   **显示控制**: 显示或隐藏、适应视图、删除、调整点大小以及重置显示设置。
*   **颜色模式**: 使用原始 RGB、指定单色或按 Z 高度生成颜色。
*   **点云信息**: 查看原始记录数、有效点数、显示点数、无效记录、RGB 状态、文件大小、包围盒和尺寸。
*   **大文件保护**: 在工作线程中读取，支持进度显示和取消；显示上限可选 500K、1M、2M、5M 或全部点，超出上限时均匀抽样。
*   **CAD/CAE 隔离**: 点云不会进入依赖拓扑的建模、测量、导出、干涉和 CAE 操作。

### 🛠️ 建模工具
*   **2D 图元**: 创建点、直线、矩形、圆、圆弧、椭圆、多边形、贝塞尔曲线 (Bezier Curve) 和 NURBS 曲线。
*   **3D 图元**: 生成立方体、球体、圆柱体、圆锥体和棱锥体。
*   **布尔运算**: 对实体进行并集 (Fuse)、交集 (Common) 和差集 (Cut) 运算。
*   **钣金连接**: 在两个选中面之间生成正交路径钣金，支持圆弧折弯、焊接或螺栓/螺母端部以及碰撞检查。
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

## 📚 文档

*   **开发与重构说明**: [`../development_refactoring.md`](../development_refactoring.md)
*   **钣金连接使用说明**: [`../tool_sheet_metal_connector.md`](../tool_sheet_metal_connector.md)
*   **钣金连接内部实现**: [`sheet_metal_connector_implementation_CN.md`](sheet_metal_connector_implementation_CN.md)
*   **钣金连接基础版与 Phase 2 计划**: [`../sheet_metal_connector_refactor_plan.md`](../sheet_metal_connector_refactor_plan.md), [`../sheet_metal_connector_phase2_plan.md`](../sheet_metal_connector_phase2_plan.md)
*   **3D PDF 导出功能**: [`../file_export_3dpdf.md`](../file_export_3dpdf.md)
*   **点云开发与手动测试计划**: [`../point_cloud_development_plan.md`](../point_cloud_development_plan.md)
*   **点云样例数据**: [`../../samples/pointcloud/README.md`](../../samples/pointcloud/README.md)
*   **其他语言 README**: [`../JP/README_JP.md`](../JP/README_JP.md), [`../FR/README_FR.md`](../FR/README_FR.md), [`../RU/README_RU.md`](../RU/README_RU.md), [`../ES/README_ES.md`](../ES/README_ES.md)

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

### 导入点云

1. 打开 **文件 > 导入**。
2. 选择 `.xyz`、`.txt`、`.pts` 或 `.ply` 文件。
3. 在 **点云** 面板中查看统计信息并调整显示设置。
4. 使用 **适应点云** 将点云完整显示在视图中。

导入其他模型或点云会替换当前场景。目前不支持 Binary Big Endian PLY、PCD、LAS/LAZ、点云配准、滤波和曲面重建。加载点云时，CAD 和 CAE 专用命令不可用。

手动测试文件位于 [`samples/pointcloud`](../../samples/pointcloud)。其中 `binary_colored_cube.ply` 用于测试属性顺序混合并包含 RGB 数据的 Binary Little Endian PLY。

## 🤝 贡献

欢迎贡献代码！如果您有建议或想要修复错误，请随时提交 Issue 或 Pull Request。
