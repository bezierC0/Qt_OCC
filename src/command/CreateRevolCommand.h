#pragma once

#include "IShapeCommand.h"
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

namespace CoreApi
{
class CreateRevolCommand : public IShapeCommand
{
public:
    void initialize(const ShapeParams& p) override;
    bool isValid() const override;
    QString name() const override;
    TopoDS_Shape execute() const override;

private:
    bool m_valid{false};
    TopoDS_Shape m_basis;
    gp_Pnt m_axisPoint;
    double m_nx{}, m_ny{}, m_nz{1.0}, m_angle{};
};
} // namespace CoreApi
