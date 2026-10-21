# Point Cloud Samples

These files exercise the point-cloud formats supported by the first version.

| File | Format | Purpose |
| --- | --- | --- |
| `cube.xyz` | XYZ | Uncolored points on a cube |
| `colored_height.xyz` | XYZRGB | Colored height surface |
| `room_scan.pts` | PTS | Count header with intensity and RGB |
| `colored_pyramid.ply` | ASCII PLY | Vertex properties and RGB colors |
| `binary_colored_cube.ply` | Binary Little Endian PLY | Mixed vertex property order, RGB colors, and face data |
| `mixed_records.xyz` | XYZ/XYZRGB | Valid points mixed with records that should be ignored |

Import the files through **File > Import** and verify the displayed point count, colors, bounds, and interaction manually. `Axle shaft.ply` is used as the larger Binary Little Endian PLY integration sample.

