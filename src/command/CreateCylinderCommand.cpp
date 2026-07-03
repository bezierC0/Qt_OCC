#include "CreateCylinderCommand.h"
#include "CommandCommon.h"
#include "ShapeFactory.h"

namespace CoreApi
{
void CreateCylinderCommand::initialize(const ShapeParams& p)
{
    m_baseCenter = gp_Pnt(p.value(Param::X).toDouble(),
                          p.value(Param::Y).toDouble(),
                          p.value(Param::Z).toDouble());
    m_radius = p.value(Param::RADIUS).toDouble();
    m_height = p.value(Param::HEIGHT).toDouble();
    m_valid = p.contains(Param::X) && p.contains(Param::RADIUS) && p.contains(Param::HEIGHT);
}

bool CreateCylinderCommand::isValid() const
{
    return m_valid;
}

QString CreateCylinderCommand::name() const
{
    return "CreateCylinder";
}

TopoDS_Shape CreateCylinderCommand::execute() const
{
    return ShapeFactory::Instance().makeCylinder(m_baseCenter, m_radius, m_height);
}
} // namespace CoreApi
