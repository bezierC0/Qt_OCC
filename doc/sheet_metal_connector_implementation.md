# Sheet Metal Connector internal implementation

_Developer guide for the base implementation in commit `a4c55fc`, updated 2026-10-23._

---

## 📋 Scope and naming

Sheet Metal Connector creates one routed sheet-metal shape between two selected faces. The UI and source identifiers use the same general-purpose name.

The implementation covers face and point selection, start-face-plane route planning, rounded bends, per-node Bend/Weld/BoltNut transitions, preview, collision checks, XCAF insertion, and in-document parameter reload. Auto Calculate rejects a colliding base route, but complete obstacle-detour search, native document persistence, unfolding, and manufacturing compensation remain outside this version.

## 🏗️ Architecture

```mermaid
flowchart LR
    accTitle: Sheet Metal Connector Architecture
    accDescr: Main data flow from interactive face selection through route planning, solid generation, collision validation, preview, and XCAF insertion

    ui[🖥️ WidgetSheetMetalConnector] --> face_geometry[📐 Face geometry]
    face_geometry --> model[📋 Connector data]
    model --> route_planner[🧭 Route planner]
    route_planner --> builder[🔧 Connector builder]
    builder --> collision_checker{🔍 Collision check}
    collision_checker -->|Acceptable| preview[👁️ Cyan preview]
    collision_checker -->|Violation| warning[⚠️ Red preview]
    preview --> apply[✅ Apply shape]
    warning --> apply
    apply --> xcaf[(💾 XCAF document)]

    classDef input fill:#dbeafe,stroke:#2563eb,stroke-width:2px,color:#1e3a5f
    classDef geometry fill:#ede9fe,stroke:#7c3aed,stroke-width:2px,color:#3b0764
    classDef decision fill:#fef9c3,stroke:#ca8a04,stroke-width:2px,color:#713f12
    classDef output fill:#dcfce7,stroke:#16a34a,stroke-width:2px,color:#14532d

    class ui,face_geometry,model input
    class route_planner,builder geometry
    class collision_checker decision
    class preview,warning,apply,xcaf output
```

| Component | Responsibility |
| --- | --- |
| `WidgetSheetMetalConnector` | UI state, pick sessions, preview, Apply/Clear, and data transfer |
| `SheetMetalConnectorModel.h` | Endpoint, joint, bend, route, and global parameter structures |
| `SheetMetalFaceGeometry` | Face-inside default point, point projection, and oriented normal |
| `SheetMetalRoutePlanner` | Start-face-plane route, duplicate removal, and validation callback |
| `SheetMetalRouteEvaluator` | Route validity, length, bend statistics, and side-bend statistics |
| `SheetMetalConnectorBuilder` | Rounded centerline, rectangular sweep, and Bolt/Nut holes |
| `SheetMetalObstacleMap` | Scene-shape extraction, exclusions, allowed-contact trimming, and bounding-box queries |
| `SheetMetalCollisionChecker` | Broad phase, exact intersection, clearance, and allowed contact |
| `ViewerWidget::displayShape` | XCAF insertion, display registration, and model-tree integration |

## 📍 Face and point selection

Face selection and point selection intentionally have different responsibilities.

### Face selection

1. `WidgetSheetMetalConnector::startPicking()` temporarily enables only the `TopAbs_FACE` selection filter.
2. `onObjectSelected()` obtains the selected sub-face and its parent `AIS_InteractiveObject`.
3. The parent `LocalTransformation()` is composed with the face location so that the face, endpoint, target shape, preview, and collision checks use world coordinates.
4. `SheetMetalFaceGeometry::centerPointAndNormal()` computes the area center with `BRepGProp::SurfaceProperties` and projects it to the underlying surface.
5. `BRepClass_FaceClassifier` accepts the point only when it is inside or on the trimmed face.
6. If the projected center lies outside a concave or holed face, an `11 x 11` UV sample searches for the valid point closest to the area center.
7. `GeomLProp_SLProps` evaluates the normal at the accepted point. A reversed face orientation reverses the normal.

### Point selection

`Pick Start Point` and `Pick End Point` read `StdSelect_ViewerSelector3d::PickedPoint(1)`, verify that the picked face is the previously stored face, and project the picked point back to that trimmed face. Only `position` changes; the stored face and normal remain unchanged.

This separation prevents the view-plane coordinate returned by `V3d_View::Convert()` from being treated as a real face hit.

## 📚 Data model

`SheetMetalConnectorData` owns the complete build input:

- `start` and `end`: world-space point, face normal, selected face, target shape, target label, and joint data
- `routePoints`: ordered endpoints and internal nodes; every internal node owns a Bend, Weld, or BoltNut transition and its parameters
- `width` and `thickness`: rectangular section dimensions
- `clearance`: required distance from non-contact obstacles
- `maximumBendCount` and `allowSideBends`: manufacturing limits applied to every continuous sheet
- `upDirection`: reference used to orient the initial rectangular section

`SheetMetalJointType` is independent from `SheetMetalBend`. Weld stores a weld-type index; BoltNut stores a specification index and hole count. The base builder currently creates one centered hole per BoltNut endpoint.

## 🎯 Per-node transition model

The implementation treats every edge between adjacent route points as a sheet segment and gives every internal node its own transition:

- `Bend`: both adjacent segments remain one continuous manufactured sheet and one solid
- `Weld`: split at the node into two sheet-metal solids and create the selected weld geometry
- `BoltNut`: split at the node into two sheet-metal solids and create the required overlap, holes, and fastener connection
- start and end attachments remain separate from internal transitions and connect the generated sheet chain to the selected target shapes

The initial route uses the start-face local plane. For start point `S`, end point `E`, and start normal `N`, the projected point is `Q = E - N * dot(E - S, N)`. The base route is `S → Q → E`; a zero-length segment is removed. A Bend at `Q` creates one folded sheet, while Weld or BoltNut creates two connected sheets. Newly added internal nodes default to Bend. Maximum bend count is evaluated per continuous sheet and resets after a non-Bend transition.

## 🧭 Route planning

`SheetMetalRoutePlanner::computeOrthogonalRoute()` currently creates the start-plane route `S → Q → E` described above. Consecutive duplicate points are removed, so a point already on the plane or normal can produce a straight two-point route. Auto Calculate builds this candidate and rejects it when the resulting solid violates collision or clearance constraints. It does not yet add detour nodes.

When the single generated corner would violate a zero-bend limit or a disabled side-bend rule, its default transition becomes Weld. Manually added nodes default to Bend; the user can change each node independently in the Selected Route Node panel.

## 🔧 Geometry construction

`SheetMetalConnectorBuilder::build()` performs these steps:

1. Validate positive width and thickness, route points, and manufacturing options.
2. Group adjacent segments through Bend nodes. Weld and BoltNut nodes terminate the current sheet and start another independent solid. A continuous sheet that exceeds `maximumBendCount`, or contains a disabled side bend, is rejected until the node transition is changed.
3. Replace every retained non-collinear corner with tangent entry/exit points and a circular arc. The tangent offset is `radius * tan(angle / 2)`; a radius is rejected when it consumes either adjacent segment.
   The radius must also exceed the rectangular section half-diagonal so that the swept bend does not fold through itself and create a stray curved face.
4. Build and sweep every sheet independently, then return one solid or a compound containing all solids.
5. A straight Butt joint shares one cut position. Lap Weld and BoltNut joints overlap the adjacent sheets by `max(10 mm, 2 × thickness)` and place them on adjacent thickness layers. Their faces contact without intersecting solid volume. BoltNut cuts aligned holes through both sheets. Corner Butt Weld sheets are trimmed by a shared miter plane to remove intersecting volume.
6. For each BoltNut endpoint, cut a cylinder along the stored face normal. M6, M8, M10, and M12 currently map to hole radii `3.3`, `4.5`, `5.5`, and `6.5` mm.

The result contains both the centerline wire and the final shape. Errors are returned as text instead of leaving a partially valid result.

## 🔍 Collision and clearance logic

`SheetMetalObstacleMap` first converts supported scene objects to world-coordinate shapes, applies exclusion and allowed-contact rules, and stores their bounding boxes. `SheetMetalCollisionChecker::check()` queries that map and performs the exact checks:

1. Build and enlarge the connector bounding box by the requested clearance.
2. Skip the connector itself and the previously applied connector shape.
3. For the two intended target shapes, subtract a spherical allowed-contact zone centered at the endpoint. Its radius is `max(width, thickness) + clearance`.
4. Reject obstacles whose bounding boxes do not overlap the enlarged connector box.
5. Use `BRepAlgoAPI_Common` to detect real intersection.
6. If there is no intersection, use `BRepExtrema_DistShapeShape` to compare the exact minimum distance with the requested clearance.

An acceptable result is displayed in cyan. Collision or clearance failure is displayed in red and blocks Apply unless the user explicitly enables the warning override.

## 🔄 Preview and Apply sequence

```mermaid
sequenceDiagram
    accTitle: Preview and Apply Sequence
    accDescr: Interaction between the panel, planner, builder, collision checker, viewer, and XCAF document during connector creation

    participant user as 👤 User
    participant panel as 🖥️ WidgetSheetMetalConnector
    participant geometry as 🔧 Planner and builder
    participant checker as 🔍 Collision checker
    participant viewer as 💾 Viewer and XCAF

    user->>panel: Select faces and parameters
    panel->>geometry: Compute route and build shape
    geometry-->>panel: Centerline and solid
    panel->>checker: Check scene objects
    checker-->>panel: Collision result
    panel-->>user: Show cyan or red preview
    user->>panel: Apply
    panel->>geometry: Rebuild final shape
    panel->>checker: Recheck final shape
    panel->>viewer: displayShape and metadata
    viewer-->>panel: TDF label
```

Preview shapes are temporary `AIS_Shape` objects. Apply rebuilds and rechecks the solid, removes the previous applied result when present, and calls `ViewerWidget::displayShape()`. The returned `TDF_Label` receives the name `Sheet Metal Connector` and a `TDataStd_Comment` metadata string.

The `version=5` metadata stores every node transition and its Bend/Weld/BoltNut parameters. The parser remains compatible with `version=1` through `version=4`; legacy global intermediate-joint settings are migrated to route nodes that require a split. When a generated connector is selected before opening the tool, the panel restores these values, resolves both parent Shape/Label references, and marks the selected connector as the result to replace on the next Apply.

The metadata does not store a stable sub-Face identifier. After reload, the saved point and normal remain usable, but changing a point requires reselecting its Face first.

## ⚠️ Limits and extension points

- Auto Calculate validates the start-face plane route, but it does not yet search for new detour points around obstacles
- Allowed contact uses a spherical approximation rather than an exact face contact region
- `holeCount` is stored but the base builder creates one centered hole
- BoltNut cannot directly connect sheets split at a corner; that case requires an angle bracket or flange planned for the next geometry extension
- Weld stores parameters but does not create a weld bead solid
- Lap Weld currently extends the route into the target by `max(10 mm, 2 × thickness)`; Phase 2 will decide between end-face contact and overlap and expose the required parameters
- Reload works inside the current XCAF document; durable native document save/reopen is not yet available
- Topological face references may need a stable sub-shape naming strategy after model changes
- The sweep represents a routed solid, not a manufacturing-grade unfolded sheet-metal definition

Phase 2 should extend the planner and evaluator without merging joint type back into bend type. See [Phase 2 plan](sheet_metal_connector_phase2_plan.md).

## 🧪 Verification

`SheetMetalConnectorTests` covers start-face-plane projection, single- and multi-sheet routes, per-node transitions, straight BoltNut sheet joints, mitered corner Weld splits, unsupported bolted corners, bend-radius rejection, collision checks, face geometry, and metadata compatibility.

Manual GUI acceptance should still verify:

- Face default points and optional point overrides on planar, curved, concave, and transformed parts
- Repeated preview, Clear, Apply, and re-Apply behavior
- Intended target contact versus illegal penetration elsewhere on the target part
- Save, reopen, metadata restoration, STEP export, and STEP re-import

## 🔗 Source map

- UI orchestration: `src/ui/widget_sheet_metal_connector.h`, `src/ui/widget_sheet_metal_connector.cpp`, `src/ui/widget_sheet_metal_connector.ui`
- Model: `src/ui/SheetMetalConnectorModel.h`
- Metadata codec: `src/ui/SheetMetalConnectorMetadata.h`, `src/ui/SheetMetalConnectorMetadata.cpp`
- Face geometry: `src/ui/SheetMetalFaceGeometry.h`, `src/ui/SheetMetalFaceGeometry.cpp`
- Route planner: `src/ui/SheetMetalRoutePlanner.h`, `src/ui/SheetMetalRoutePlanner.cpp`
- Route evaluator: `src/ui/SheetMetalRouteEvaluator.h`, `src/ui/SheetMetalRouteEvaluator.cpp`
- Builder: `src/ui/SheetMetalConnectorBuilder.h`, `src/ui/SheetMetalConnectorBuilder.cpp`
- Obstacle map: `src/ui/SheetMetalObstacleMap.h`, `src/ui/SheetMetalObstacleMap.cpp`
- Collision checker: `src/ui/SheetMetalCollisionChecker.h`, `src/ui/SheetMetalCollisionChecker.cpp`
- Tests: `tests/SheetMetalConnectorTests.cpp`
- UI translations: `resource/i18n/qt_occ_en.ts`, `qt_occ_zh.ts`, `qt_occ_ja.ts`
