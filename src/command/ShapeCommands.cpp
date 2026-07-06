#include "ShapeCommands.h"
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

void registerShapeCommands()
{
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;

    auto& reg = ShapeCommandRegistry::instance();
    reg.registerCommand("CreatePoint",     []() -> std::unique_ptr<IShapeCommand> { return std::make_unique<CreatePointCommand>(); });
    reg.registerCommand("CreateLine",      []() -> std::unique_ptr<IShapeCommand> { return std::make_unique<CreateLineCommand>(); });
    reg.registerCommand("CreateRectangle", []() -> std::unique_ptr<IShapeCommand> { return std::make_unique<CreateRectangleCommand>(); });
    reg.registerCommand("CreateCircle",    []() -> std::unique_ptr<IShapeCommand> { return std::make_unique<CreateCircleCommand>(); });
    reg.registerCommand("CreateArc",       []() -> std::unique_ptr<IShapeCommand> { return std::make_unique<CreateArcCommand>(); });
    reg.registerCommand("CreateEllipse",   []() -> std::unique_ptr<IShapeCommand> { return std::make_unique<CreateEllipseCommand>(); });
    reg.registerCommand("CreatePolygon",   []() -> std::unique_ptr<IShapeCommand> { return std::make_unique<CreatePolygonCommand>(); });
    reg.registerCommand("CreateBezier",    []() -> std::unique_ptr<IShapeCommand> { return std::make_unique<CreateBezierCommand>(); });
    reg.registerCommand("CreateNurbs",     []() -> std::unique_ptr<IShapeCommand> { return std::make_unique<CreateNurbsCommand>(); });
    reg.registerCommand("CreateBox",       []() -> std::unique_ptr<IShapeCommand> { return std::make_unique<CreateBoxCommand>(); });
    reg.registerCommand("CreateSphere",    []() -> std::unique_ptr<IShapeCommand> { return std::make_unique<CreateSphereCommand>(); });
    reg.registerCommand("CreateCylinder",  []() -> std::unique_ptr<IShapeCommand> { return std::make_unique<CreateCylinderCommand>(); });
    reg.registerCommand("CreateCone",      []() -> std::unique_ptr<IShapeCommand> { return std::make_unique<CreateConeCommand>(); });
}

} // namespace CoreApi
