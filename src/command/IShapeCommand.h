#pragma once

#include <TopoDS_Shape.hxx>
#include <QColor>
#include <QString>
#include <QVariantMap>

namespace CoreApi {

/**
 * @brief Parameter bag passed from Dialog -> Command.
 *
 * Uses QVariantMap so the command layer has no dependency on Qt widget types beyond QVariant. The Dialog fills it; the Command reads it.
 */
using ShapeParams = QVariantMap;

/**
 * @brief Interface for all shape-creation commands.
 *
 * Mirrors the ICommand pattern from ECAD-3D2:
 *   - initialize() validates and stores parameters
 *   - isValid()    guards execute()
 *   - execute()    builds and returns the shape
 *
 * No Qt widgets, no display logic - pure geometry.
 */
class IShapeCommand
{
public:
    virtual ~IShapeCommand() = default;

    virtual void        initialize(const ShapeParams& params) = 0;
    virtual bool        isValid()  const = 0;
    virtual TopoDS_Shape execute() const = 0;
    virtual QString     name()     const = 0;
};

} // namespace CoreApi
