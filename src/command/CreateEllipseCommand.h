#pragma once
#include "CommandCommon.h"
#include "ShapeCommandRegistry.h"
#include "ShapeFactory.h"

#include <gp_Pnt.hxx>
namespace CoreApi
{
// ---------------------------------------------------------------------------
// CreateEllipseCommand
// ---------------------------------------------------------------------------
class CreateEllipseCommand : public IShapeCommand
{
public:
    void initialize(const ShapeParams &p) override;
    bool isValid() const override;
    QString name() const override;
    TopoDS_Shape execute() const override;

private:
    bool m_valid{false};
    gp_Pnt m_center;
    double m_nx{}, m_ny{}, m_nz{1.0};
    double m_major{}, m_minor{};
};
} // namespace CoreApi