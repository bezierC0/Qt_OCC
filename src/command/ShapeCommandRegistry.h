#pragma once

#include "IShapeCommand.h"

#include <TopoDS_Shape.hxx>
#include <QString>
#include <functional>
#include <memory>
#include <unordered_map>

namespace CoreApi {

/**
 * @brief Singleton registry that maps command names to factory functions.
 *
 * The caller (ViewerWidget) only knows the command name string - no
 * compile-time dependency on any concrete command class.
 *
 * Registration is done once in ShapeCommands.cpp via a static initializer.
 *
 * Usage:
 *   auto shape = ShapeCommandRegistry::instance().execute("CreatePoint", params);
 */
class ShapeCommandRegistry
{
public:
    using Factory = std::function<std::unique_ptr<IShapeCommand>()>;

    static ShapeCommandRegistry& instance();

    void registerCommand(const QString& name, Factory factory);

    /// Build, initialize, validate and execute a command in one call.
    /// Returns a null shape on failure (unknown name, invalid params, build error).
    TopoDS_Shape execute(const QString& name, const ShapeParams& params) const;

private:
    ShapeCommandRegistry() = default;
    std::unordered_map<QString, Factory> m_registry;
};

} // namespace CoreApi
