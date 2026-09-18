#include "CreateRevolCommand.h"
#include "CommandCommon.h"
#include "ShapeFactory.h"

namespace CoreApi
{
void CreateRevolCommand::initialize(const ShapeParams& p)
{
    m_valid = p.contains(Param::BASIS) && p.contains(Param::X) && p.contains(Param::Y)
              && p.contains(Param::Z) && p.contains(Param::NX) && p.contains(Param::NY)
              && p.contains(Param::NZ) && p.contains(Param::ANGLE);
    if (!m_valid) return;
    m_basis = p.value(Param::BASIS).value<TopoDS_Shape>();
    m_axisPoint = gp_Pnt(p.value(Param::X).toDouble(),
                         p.value(Param::Y).toDouble(),
                         p.value(Param::Z).toDouble());
    m_nx = p.value(Param::NX).toDouble();
    m_ny = p.value(Param::NY).toDouble();
    m_nz = p.value(Param::NZ).toDouble();
    m_angle = p.value(Param::ANGLE).toDouble();
}

bool CreateRevolCommand::isValid() const { return m_valid; }
QString CreateRevolCommand::name() const { return "CreateRevol"; }

TopoDS_Shape CreateRevolCommand::execute() const
{
    return ShapeFactory::Instance().makeRevol(m_basis, m_axisPoint, m_nx, m_ny, m_nz, m_angle);
}
} // namespace CoreApi
