#include "IShapeCommand.h"
#include "ShapeCommandRegistry.h"
#include "ShapeFactory.h"
#include "command/CreateCircleCommand.h"
#include "command/CreateEllipseCommand.h"
#include "command/CreateLineCommand.h"
#include "command/CreatePointCommand.h"
#include "command/CreateRectangleCommand.h"

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
            reg.registerCommand("CreateEllipse",   []() -> std::unique_ptr<CoreApi::IShapeCommand> { return std::make_unique<CoreApi::CreateEllipseCommand>(); });
        }
    } g_shapeCommandsRegistrar;
}
