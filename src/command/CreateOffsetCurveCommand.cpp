#include "CreateOffsetCurveCommand.h"
#include "CommandCommon.h"
#include "ShapeFactory.h"

namespace CoreApi
{
void CreateOffsetCurveCommand::initialize(const ShapeParams& p)
{
    m_valid = p.contains(Param::BASIS) && p.contains(Param::DISTANCE);
    if (!m_valid) return;
    m_basis = p.value(Param::BASIS).value<TopoDS_Shape>();
    m_distance = p.value(Param::DISTANCE).toDouble();
    m_nx = p.value(Param::NX, 0.0).toDouble();
    m_ny = p.value(Param::NY, 0.0).toDouble();
    m_nz = p.value(Param::NZ, 1.0).toDouble();
}

bool CreateOffsetCurveCommand::isValid() const { return m_valid; }
QString CreateOffsetCurveCommand::name() const { return "CreateOffsetCurve"; }

TopoDS_Shape CreateOffsetCurveCommand::execute() const
{
    return ShapeFactory::Instance().makeOffsetCurve(m_basis, m_distance, m_nx, m_ny, m_nz);
}
} // namespace CoreApi
