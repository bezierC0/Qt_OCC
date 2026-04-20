#include "CreateLineCommand.h"

namespace CoreApi
{
void CreateLineCommand::initialize(const ShapeParams &p)
{
    m_p1 = gp_Pnt(p.value(Param::X1).toDouble(),
                  p.value(Param::Y1).toDouble(),
                  p.value(Param::Z1).toDouble());
    m_p2 = gp_Pnt(p.value(Param::X2).toDouble(),
                  p.value(Param::Y2).toDouble(),
                  p.value(Param::Z2).toDouble());
    m_valid = p.contains(Param::X1) && p.contains(Param::X2);
}

bool CreateLineCommand::isValid() const
{
    return m_valid;
}

QString CreateLineCommand::name() const
{
    return "CreateLine";
}

TopoDS_Shape CreateLineCommand::execute() const
{
    return ShapeFactory::makeLine(m_p1, m_p2);
}
} // namespace CoreApi