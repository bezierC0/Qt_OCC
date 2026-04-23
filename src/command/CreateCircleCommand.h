#pragma once
#include "IShapeCommand.h"
#include <gp_Pnt.hxx>
#include <QString>
namespace CoreApi
{
// ---------------------------------------------------------------------------
// CreateCircleCommand
// ---------------------------------------------------------------------------
class CreateCircleCommand : public IShapeCommand
{
public:
    void initialize(const ShapeParams &p) override;
    bool isValid() const override;
    QString name() const override;
    TopoDS_Shape execute() const override;

private:
    bool m_valid{false};
    gp_Pnt m_center;
    double m_radius{};
};
} // namespace CoreApi