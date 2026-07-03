#include "CreateBezierCommand.h"
#include "CommandCommon.h"
#include "ShapeFactory.h"

namespace CoreApi
{
void CreateBezierCommand::initialize(const ShapeParams& p)
{
    const PointList points = p.value(Param::POINTS).value<PointList>();
    m_points.assign(points.begin(), points.end());
    m_valid = p.contains(Param::POINTS) && m_points.size() >= 2;
}

bool CreateBezierCommand::isValid() const
{
    return m_valid;
}

QString CreateBezierCommand::name() const
{
    return "CreateBezier";
}

TopoDS_Shape CreateBezierCommand::execute() const
{
    return ShapeFactory::Instance().makeBezierCurve(m_points);
}
} // namespace CoreApi
