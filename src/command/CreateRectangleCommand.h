#pragma once
#include "CommandCommon.h"
#include "ShapeCommandRegistry.h"
#include "ShapeFactory.h"

#include <gp_Pnt.hxx>
namespace CoreApi
{

// ---------------------------------------------------------------------------
// CreateRectangleCommand
// ---------------------------------------------------------------------------
class CreateRectangleCommand : public IShapeCommand
{
public:
    void initialize(const ShapeParams &p) override;
    bool isValid() const override;
    QString name() const override;
    TopoDS_Shape execute() const override;

private:
    bool m_valid{false};
    gp_Pnt m_origin;
    double m_width{}, m_height{};
};
} // namespace CoreApi