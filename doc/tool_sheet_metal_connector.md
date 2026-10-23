# Sheet Metal Connector

Sheet Metal Connector creates a routed sheet-metal solid between two selected model faces.

## Basic workflow

1. Open **Other Tools → Sheet Metal Connector**.
2. Select the start Face. A default connection point is placed inside that Face.
3. Select the end Face.
4. Optionally use the Point buttons to choose another point on the corresponding selected Face.
5. Set width, thickness, clearance, maximum bends per sheet, side-bend permission, and the Weld or Bolt/Nut mode for each end. The base version locks the initial sheet-thickness direction to the Start face normal.
6. Click **Auto Calculate Route**. The tool creates a start-face-plane candidate and accepts it only when the first and last panels are parallel to their selected faces without twisting the rectangular section.
7. Select an internal route node to choose Bend, Weld, or Bolt/Nut and edit its parameters. New waypoints default to Bend.
8. Confirm the preview. Cyan is acceptable; red indicates collision or insufficient clearance.
9. Click **Apply**.

## Edit an existing connector

Select the generated **Sheet Metal Connector** shape, then open the tool again. The tool restores its dimensions, route, joint settings, and parent target references. The next Apply replaces the selected connector.

The base version does not persist a stable sub-Face identifier. To change a restored endpoint, reselect its Face before using the Point button.

## Current limits

- Route planning validates the start-face plane route, but it does not yet create additional detour points around an obstacle.
- The strict base version rejects a route unless its first segment lies in the Start face plane, its last segment lies in the End face plane, and its transported final sheet normal is parallel to the End face normal. It does not yet insert extra panels to repair an incompatible orientation.
- Bend keeps adjacent segments in one continuous sheet. Weld or Bolt/Nut starts another sheet-metal shape at that node.
- A continuous sheet that exceeds its bend limit or contains a disabled side bend must use Weld or Bolt/Nut at a suitable node. Auto Calculate converts its single generated corner to Weld when required.
- A bolted corner requires an angle bracket or flange and is not generated directly.
- Bolt/Nut creates a basic connection hole but not standard fastener solids.
- Butt Weld ends at the selected Face. Lap Weld currently extends into each target by `max(10 mm, 2 × thickness)` but does not create a weld bead solid.
- Native document save/reopen and manufacturing-grade unfolding are not yet available.

For implementation details, see [Sheet Metal Connector internal implementation](sheet_metal_connector_implementation.md).
