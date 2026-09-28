#include "CreateTorusCommand.h"
#include "CommandCommon.h"
#include "ShapeFactory.h"

namespace CoreApi
{
void CreateTorusCommand::initialize(const ShapeParams& p)
{
    m_center = gp_Pnt(p.value(Param::X).toDouble(),
                      p.value(Param::Y).toDouble(),
                      p.value(Param::Z).toDouble());
    m_majorRadius = p.value(Param::MAJOR).toDouble();
    m_minorRadius = p.value(Param::MINOR).toDouble();
    m_valid = p.contains(Param::X) && p.contains(Param::MAJOR) && p.contains(Param::MINOR);
}

bool CreateTorusCommand::isValid() const
{
    return m_valid;
}

QString CreateTorusCommand::name() const
{
    return "CreateTorus";
}

TopoDS_Shape CreateTorusCommand::execute() const
{
    return ShapeFactory::Instance().makeTorus(m_center, m_majorRadius, m_minorRadius);
}
} // namespace CoreApi
