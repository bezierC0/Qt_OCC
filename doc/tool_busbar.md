# 3D Busbar Routing Feature

This application includes a feature for **3D Busbar Routing**, designed primarily to assist in switchgear engineering and design validation. 

## Core Concepts

The Busbar routing tool is built around the following geometric and manufacturing constraints:

* **Orthogonal Paths**: Routes must consist of line segments strictly parallel to the X, Y, or Z axes. Diagonal lines are not permitted.
* **Waypoints**: Nodes along the path represent the start point, end point, and intermediate bends. Waypoints can be automatically calculated or manually edited by the user.
* **Connection Types**: Each waypoint can be assigned specific manufacturing properties, including **Bend** (requires bend radius/angle), **Bolt** (requires thread size), or **Weld** (requires joint type).

## Internal Workflow

The feature operates on an Apply-based modeling flow:

1. **Endpoint Selection**: The user selects the start and end positions, including their respective normal directions.
2. **Automatic Path Calculation**: The system evaluates up to 6 candidate orthogonal permutations (e.g., X→Y→Z, Z→X→Y). It selects the optimal path by prioritizing the shortest total length, minimum number of bends, and best alignment with the endpoint normals.
3. **Waypoint Editing**: Users can refine the automatically generated path by adding, removing, or moving waypoints. Movement is strictly constrained to specific axes or planes to maintain orthogonality.
4. **Parameter Configuration**: The user assigns connection properties to the waypoints and defines the global busbar parameters (width, thickness, and cross-section orientation).
5. **3D Model Generation**: Upon clicking "Apply", the system constructs the 3D geometry using OpenCASCADE by sweeping the rectangular cross-section profile along the defined wire path.


