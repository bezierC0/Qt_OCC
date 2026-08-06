#include "CreateConeCommand.h"
#include "CommandCommon.h"
#include "ShapeFactory.h"

namespace CoreApi
{
void CreateConeCommand::initialize(const ShapeParams& p)
{
    m_baseCenter = gp_Pnt(p.value(Param::X).toDouble(),
                          p.value(Param::Y).toDouble(),
                          p.value(Param::Z).toDouble());
    m_radius1 = p.value(Param::RADIUS1).toDouble();
    m_radius2 = p.value(Param::RADIUS2).toDouble();
    m_height = p.value(Param::HEIGHT).toDouble();
    m_valid = p.contains(Param::X) && p.contains(Param::RADIUS1) &&
              p.contains(Param::RADIUS2) && p.contains(Param::HEIGHT);
}

bool CreateConeCommand::isValid() const
{
    return m_valid;
}

QString CreateConeCommand::name() const
{
    return "CreateCone";
}

TopoDS_Shape CreateConeCommand::execute() const
{
    return ShapeFactory::Instance().makeCone(m_baseCenter, m_radius1, m_radius2, m_height);
}
} // namespace CoreApi
