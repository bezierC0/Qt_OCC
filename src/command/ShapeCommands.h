#pragma once

// Forward-declare all concrete shape commands.
// Include this header only where you need the types directly
// (e.g. unit tests). Normal callers go through ShapeCommandRegistry.

namespace CoreApi {
class CreatePointCommand;
class CreateLineCommand;
class CreateRectangleCommand;
class CreateCircleCommand;
class CreateEllipseCommand;
} // namespace CoreApi
