#include "CreateSphereCommand.h"
#include "CommandCommon.h"
#include "ShapeFactory.h"

namespace CoreApi
{
void CreateSphereCommand::initialize(const ShapeParams& p)
{
    m_center = gp_Pnt(p.value(Param::X).toDouble(),
                      p.value(Param::Y).toDouble(),
                      p.value(Param::Z).toDouble());
    m_radius = p.value(Param::RADIUS).toDouble();
    m_valid = p.contains(Param::X) && p.contains(Param::RADIUS);
}

bool CreateSphereCommand::isValid() const
{
    return m_valid;
}

QString CreateSphereCommand::name() const
{
    return "CreateSphere";
}

TopoDS_Shape CreateSphereCommand::execute() const
{
    return ShapeFactory::Instance().makeSphere(m_center, m_radius);
}
} // namespace CoreApi
