# Sheet Metal Connector

Sheet Metal Connector creates a routed sheet-metal solid between two selected model faces.

## Basic workflow

1. Open **Other Tools → Sheet Metal Connector**.
2. Select the start Face. A default connection point is placed inside that Face.
3. Select the end Face.
4. Optionally use the Point buttons to choose another point on the corresponding selected Face.
5. Set width, thickness, clearance, section direction, and the Weld or Bolt/Nut mode for each end.
6. Click **Auto Calculate Route**, then adjust bend radii or waypoints if needed.
7. Confirm the preview. Cyan is acceptable; red indicates collision or insufficient clearance.
8. Click **Apply**.

## Edit an existing connector

Select the generated **Sheet Metal Connector** shape, then open the tool again. The tool restores its dimensions, route, joint settings, and parent target references. The next Apply replaces the selected connector.

The base version does not persist a stable sub-Face identifier. To change a restored endpoint, reselect its Face before using the Point button.

## Current limits

- Route planning does not automatically avoid obstacles.
- Bolt/Nut creates a basic connection hole but not standard fastener solids.
- Weld settings are stored but do not create a weld bead solid.
- Native document save/reopen and manufacturing-grade unfolding are not yet available.

For implementation details, see [Sheet Metal Connector internal implementation](sheet_metal_connector_implementation.md).
