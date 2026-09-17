#include "CreateParabolaCommand.h"
#include "CommandCommon.h"
#include "ShapeCommandRegistry.h"
#include "ShapeFactory.h"
namespace CoreApi
{
void CreateParabolaCommand::initialize(const ShapeParams &p)
{
    m_vertex = gp_Pnt(p.value(Param::X).toDouble(), p.value(Param::Y).toDouble(),
                      p.value(Param::Z).toDouble());
    m_nx = p.value(Param::NX, 0.0).toDouble();
    m_ny = p.value(Param::NY, 0.0).toDouble();
    m_nz = p.value(Param::NZ, 1.0).toDouble();
    m_focal = p.value(Param::FOCAL, 0.0).toDouble();
    m_first = p.value(Param::FIRST_PARAMETER, -30.0).toDouble();
    m_last = p.value(Param::LAST_PARAMETER, 30.0).toDouble();
    m_valid = p.contains(Param::FOCAL);
}

bool CreateParabolaCommand::isValid() const
{
    return m_valid;
}

QString CreateParabolaCommand::name() const
{
    return "CreateParabola";
}

TopoDS_Shape CreateParabolaCommand::execute() const
{
    return ShapeFactory::Instance().makeParabola(m_vertex, m_nx, m_ny, m_nz, m_focal, m_first, m_last);
}
} // namespace CoreApi