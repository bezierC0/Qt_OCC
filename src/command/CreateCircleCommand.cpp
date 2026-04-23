#include "CreateCircleCommand.h"
#include "CommandCommon.h"
#include "ShapeCommandRegistry.h"
#include "ShapeFactory.h"

namespace CoreApi
{
void CreateCircleCommand::initialize(const ShapeParams &p)
{
    m_center = gp_Pnt(p.value(Param::X).toDouble(), p.value(Param::Y).toDouble(),
                      p.value(Param::Z).toDouble());
    m_radius = p.value(Param::RADIUS, 0.0).toDouble();
    m_valid = p.contains(Param::RADIUS);
}

bool CreateCircleCommand::isValid() const
{
    return m_valid;
}

QString CreateCircleCommand::name() const
{
    return "CreateCircle";
}

TopoDS_Shape CreateCircleCommand::execute() const
{
    return ShapeFactory::Instance().makeCircle(m_center, m_radius);
}
} // namespace CoreApi