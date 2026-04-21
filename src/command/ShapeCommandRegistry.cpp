#include "ShapeCommandRegistry.h"

namespace CoreApi {

ShapeCommandRegistry& ShapeCommandRegistry::instance()
{
    static ShapeCommandRegistry s;
    return s;
}

void ShapeCommandRegistry::registerCommand(const QString& name, Factory factory)
{
    m_registry[name] = std::move(factory);
}

TopoDS_Shape ShapeCommandRegistry::execute(const QString& name, const ShapeParams& params) const
{
    auto it = m_registry.find(name);
    if (it == m_registry.end()) return {};

    auto cmd = it->second();
    cmd->initialize(params);
    if (!cmd->isValid()) return {};

    return cmd->execute();
}

} // namespace CoreApi
