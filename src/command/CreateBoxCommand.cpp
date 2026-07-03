#include "CreateBoxCommand.h"
#include "CommandCommon.h"
#include "ShapeFactory.h"

namespace CoreApi
{
void CreateBoxCommand::initialize(const ShapeParams& p)
{
    m_corner = gp_Pnt(p.value(Param::X).toDouble(),
                      p.value(Param::Y).toDouble(),
                      p.value(Param::Z).toDouble());
    m_dx = p.value(Param::DX).toDouble();
    m_dy = p.value(Param::DY).toDouble();
    m_dz = p.value(Param::DZ).toDouble();
    m_valid = p.contains(Param::X) && p.contains(Param::DX) && p.contains(Param::DY) && p.contains(Param::DZ);
}

bool CreateBoxCommand::isValid() const
{
    return m_valid;
}

QString CreateBoxCommand::name() const
{
    return "CreateBox";
}

TopoDS_Shape CreateBoxCommand::execute() const
{
    return ShapeFactory::Instance().makeBox(m_corner, m_dx, m_dy, m_dz);
}
} // namespace CoreApi
