#include "CreateNurbsCommand.h"
#include "CommandCommon.h"
#include "ShapeFactory.h"

namespace CoreApi
{
void CreateNurbsCommand::initialize(const ShapeParams& p)
{
    const PointList points = p.value(Param::POINTS).value<PointList>();
    m_points.assign(points.begin(), points.end());
    m_degree = p.value(Param::DEGREE, 0).toInt();
    m_valid = p.contains(Param::POINTS) && p.contains(Param::DEGREE) &&
              m_degree >= 1 && m_points.size() > static_cast<size_t>(m_degree);
}

bool CreateNurbsCommand::isValid() const
{
    return m_valid;
}

QString CreateNurbsCommand::name() const
{
    return "CreateNurbs";
}

TopoDS_Shape CreateNurbsCommand::execute() const
{
    return ShapeFactory::Instance().makeNurbsCurve(m_points, m_degree);
}
} // namespace CoreApi
