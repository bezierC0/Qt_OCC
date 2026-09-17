#pragma once
#include "IShapeCommand.h"
#include <gp_Pnt.hxx>
#include <QString>
namespace CoreApi
{
class CreateParabolaCommand : public IShapeCommand
{
public:
    void initialize(const ShapeParams &p) override;
    bool isValid() const override;
    QString name() const override;
    TopoDS_Shape execute() const override;

private:
    bool m_valid{false};
    gp_Pnt m_vertex;
    double m_nx{}, m_ny{}, m_nz{1.0};
    double m_focal{};
    double m_first{-30.0}, m_last{30.0};
};
} // namespace CoreApi