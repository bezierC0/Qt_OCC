#pragma once
#include "IShapeCommand.h"

namespace CoreApi
{
class CreateOffsetCurveCommand : public IShapeCommand
{
public:
    void initialize(const ShapeParams& p) override;
    bool isValid() const override;
    QString name() const override;
    TopoDS_Shape execute() const override;

private:
    bool m_valid{false};
    TopoDS_Shape m_basis;
    double m_distance{};
    double m_nx{}, m_ny{}, m_nz{1.0};
};
} // namespace CoreApi
