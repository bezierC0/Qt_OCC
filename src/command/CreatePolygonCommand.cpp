#include "CreatePolygonCommand.h"
#include "CommandCommon.h"
#include "ShapeFactory.h"

namespace CoreApi
{
void CreatePolygonCommand::initialize(const ShapeParams& p)
{
    const PointList points = p.value(Param::POINTS).value<PointList>();
    m_points.assign(points.begin(), points.end());
    m_closed = p.value(Param::CLOSED, false).toBool();
    m_valid = p.contains(Param::POINTS) && m_points.size() >= 2;
}

bool CreatePolygonCommand::isValid() const
{
    return m_valid;
}

QString CreatePolygonCommand::name() const
{
    return "CreatePolygon";
}

TopoDS_Shape CreatePolygonCommand::execute() const
{
    return ShapeFactory::Instance().makePolygonWire(m_points, m_closed);
}
} // namespace CoreApi
