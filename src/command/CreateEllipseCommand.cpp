#include "CreateEllipseCommand.h"
#include "CommandCommon.h"
#include "ShapeCommandRegistry.h"
#include "ShapeFactory.h"
namespace CoreApi
{
void CreateEllipseCommand::initialize(const ShapeParams &p)
{
    m_center = gp_Pnt(p.value(Param::X).toDouble(), p.value(Param::Y).toDouble(),
                      p.value(Param::Z).toDouble());
    m_nx = p.value(Param::NX, 0.0).toDouble();
    m_ny = p.value(Param::NY, 0.0).toDouble();
    m_nz = p.value(Param::NZ, 1.0).toDouble();
    m_major = p.value(Param::MAJOR, 0.0).toDouble();
    m_minor = p.value(Param::MINOR, 0.0).toDouble();
    m_valid = p.contains(Param::MAJOR) && p.contains(Param::MINOR);
}

bool CreateEllipseCommand::isValid() const
{
    return m_valid;
}

QString CreateEllipseCommand::name() const
{
    return "CreateEllipse";
}

TopoDS_Shape CreateEllipseCommand::execute() const
{
    return ShapeFactory::Instance().makeEllipse(m_center, m_nx, m_ny, m_nz, m_major, m_minor);
}
} // namespace CoreApi