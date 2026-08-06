#include "CreateArcCommand.h"
#include "CommandCommon.h"
#include "ShapeFactory.h"

namespace CoreApi
{
void CreateArcCommand::initialize(const ShapeParams& p)
{
    m_p1 = gp_Pnt(p.value(Param::X1).toDouble(),
                  p.value(Param::Y1).toDouble(),
                  p.value(Param::Z1).toDouble());
    m_p2 = gp_Pnt(p.value(Param::X2).toDouble(),
                  p.value(Param::Y2).toDouble(),
                  p.value(Param::Z2).toDouble());
    m_p3 = gp_Pnt(p.value(Param::X3).toDouble(),
                  p.value(Param::Y3).toDouble(),
                  p.value(Param::Z3).toDouble());
    m_valid = p.contains(Param::X1) && p.contains(Param::X2) && p.contains(Param::X3);
}

bool CreateArcCommand::isValid() const
{
    return m_valid;
}

QString CreateArcCommand::name() const
{
    return "CreateArc";
}

TopoDS_Shape CreateArcCommand::execute() const
{
    return ShapeFactory::Instance().makeArc(m_p1, m_p2, m_p3);
}
} // namespace CoreApi
