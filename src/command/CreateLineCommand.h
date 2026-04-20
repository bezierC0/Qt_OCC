#pragma once
#include "CommandCommon.h"
#include "ShapeCommandRegistry.h"
#include "ShapeFactory.h"

#include <gp_Pnt.hxx>
namespace CoreApi{
// ---------------------------------------------------------------------------
// CreateLineCommand
// ---------------------------------------------------------------------------
class CreateLineCommand : public IShapeCommand
{
public:
    void initialize(const ShapeParams& p) override;
    bool isValid() const override;
    QString name() const override;
    TopoDS_Shape execute() const override;

private:
    bool    m_valid{false};
    gp_Pnt  m_p1, m_p2;
};
}