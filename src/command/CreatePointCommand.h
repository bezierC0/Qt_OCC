#pragma once
#include "CommandCommon.h"
#include "ShapeCommandRegistry.h"
#include "ShapeFactory.h"

#include <gp_Pnt.hxx>
namespace CoreApi
{
// ---------------------------------------------------------------------------
// CreatePointCommand
// ---------------------------------------------------------------------------
class CreatePointCommand : public IShapeCommand
{
public:
    void initialize(const ShapeParams &p) override;
    bool isValid() const override;
    QString name() const override;
    TopoDS_Shape execute() const override;

private:
    bool m_valid{false};
    double m_x{}, m_y{}, m_z{};
};
} // namespace CoreApi