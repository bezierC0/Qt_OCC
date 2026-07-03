#pragma once

#include "IShapeCommand.h"
#include <gp_Pnt.hxx>

namespace CoreApi
{
class CreateCylinderCommand : public IShapeCommand
{
public:
    void initialize(const ShapeParams& p) override;
    bool isValid() const override;
    QString name() const override;
    TopoDS_Shape execute() const override;

private:
    bool m_valid{false};
    gp_Pnt m_baseCenter;
    double m_radius{};
    double m_height{};
};
} // namespace CoreApi
