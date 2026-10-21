#pragma once

#include "SheetMetalConnectorModel.h"

#include <QString>

class SheetMetalRoutePlanner
{
public:
    static bool computeOrthogonalRoute(SheetMetalConnectorData& data,
                                       double bendRadius,
                                       QString* errorMessage = nullptr);
};
