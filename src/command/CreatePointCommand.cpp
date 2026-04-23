#include "CreatePointCommand.h"
#include "CommandCommon.h"
#include "ShapeCommandRegistry.h"
#include "ShapeFactory.h"
namespace CoreApi
{
void CreatePointCommand::initialize(const ShapeParams &p)
{
    m_x = p.value(Param::X, 0.0).toDouble();
    m_y = p.value(Param::Y, 0.0).toDouble();
    m_z = p.value(Param::Z, 0.0).toDouble();
    m_valid = p.contains(Param::X);
}

bool CreatePointCommand::isValid() const
{
    return m_valid;
}

QString CreatePointCommand::name() const
{
    return "CreatePoint";
}

TopoDS_Shape CreatePointCommand::execute() const
{
    return ShapeFactory::Instance().makePoint(m_x, m_y, m_z);
}
} // namespace CoreApi