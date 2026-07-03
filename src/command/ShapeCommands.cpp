#include "IShapeCommand.h"
#include "ShapeCommandRegistry.h"
#include "ShapeFactory.h"
#include "command/CreateArcCommand.h"
#include "command/CreateBezierCommand.h"
#include "command/CreateBoxCommand.h"
#include "command/CreateCircleCommand.h"
#include "command/CreateConeCommand.h"
#include "command/CreateCylinderCommand.h"
#include "command/CreateEllipseCommand.h"
#include "command/CreateLineCommand.h"
#include "command/CreateNurbsCommand.h"
#include "command/CreatePointCommand.h"
#include "command/CreatePolygonCommand.h"
#include "command/CreateRectangleCommand.h"
#include "command/CreateSphereCommand.h"

#include <gp_Pnt.hxx>

namespace CoreApi {

} // namespace CoreApi

// ---------------------------------------------------------------------------
// Self-registrations - executed at static init time
// ---------------------------------------------------------------------------
namespace {
    struct ShapeCommandsRegistrar {
        ShapeCommandsRegistrar() {
            auto& reg = CoreApi::ShapeCommandRegistry::instance();
            reg.registerCommand("CreatePoint",     []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreatePointCommand>(); });
            reg.registerCommand("CreateLine",      []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreateLineCommand>(); });
            reg.registerCommand("CreateRectangle", []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreateRectangleCommand>(); });
            reg.registerCommand("CreateCircle",    []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreateCircleCommand>(); });
            reg.registerCommand("CreateArc",       []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreateArcCommand>(); });
            reg.registerCommand("CreateEllipse",   []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreateEllipseCommand>(); });
            reg.registerCommand("CreatePolygon",   []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreatePolygonCommand>(); });
            reg.registerCommand("CreateBezier",    []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreateBezierCommand>(); });
            reg.registerCommand("CreateNurbs",     []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreateNurbsCommand>(); });
            reg.registerCommand("CreateBox",       []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreateBoxCommand>(); });
            reg.registerCommand("CreateSphere",    []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreateSphereCommand>(); });
            reg.registerCommand("CreateCylinder",  []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreateCylinderCommand>(); });
            reg.registerCommand("CreateCone",      []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreateConeCommand>(); });
        }
    } g_shapeCommandsRegistrar;
}
