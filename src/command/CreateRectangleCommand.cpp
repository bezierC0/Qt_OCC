#include "CreateRectangleCommand.h"
#include "CommandCommon.h"
#include "ShapeCommandRegistry.h"
#include "ShapeFactory.h"
namespace CoreApi
{
void CreateRectangleCommand::initialize(const ShapeParams &p)
{
    m_origin = gp_Pnt(p.value(Param::X).toDouble(), p.value(Param::Y).toDouble(),
                      p.value(Param::Z).toDouble());
    m_width = p.value(Param::WIDTH, 0.0).toDouble();
    m_height = p.value(Param::HEIGHT, 0.0).toDouble();
    m_valid = p.contains(Param::WIDTH) && p.contains(Param::HEIGHT);
}

bool CreateRectangleCommand::isValid() const
{
    return m_valid;
}

QString CreateRectangleCommand::name() const
{
    return "CreateRectangle";
}

TopoDS_Shape CreateRectangleCommand::execute() const
{
    return ShapeFactory::Instance().makeRectangleWire(m_origin, m_width, m_height);
}
} // namespace CoreApi