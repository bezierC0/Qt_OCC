#pragma once

// Forward-declare all concrete shape commands.
// Include this header only where you need the types directly
// (e.g. unit tests). Normal callers go through ShapeCommandRegistry.

namespace CoreApi {
class CreatePointCommand;
class CreateLineCommand;
class CreateRectangleCommand;
class CreateCircleCommand;
class CreateArcCommand;
class CreateEllipseCommand;
class CreatePolygonCommand;
class CreateBezierCommand;
class CreateNurbsCommand;
class CreateBoxCommand;
class CreateSphereCommand;
class CreateCylinderCommand;
class CreateConeCommand;

void registerShapeCommands();
} // namespace CoreApi
