#pragma once

#include "SheetMetalConnectorModel.h"

#include <QString>

#include <functional>

class SheetMetalRoutePlanner
{
public:
    using RouteValidator = std::function<bool(const SheetMetalConnectorData&)>;

    static bool computeOrthogonalRoute(SheetMetalConnectorData& data,
                                       double bendRadius,
                                       QString* errorMessage = nullptr,
                                       const RouteValidator& routeValidator = {});
    static bool validateRoute(const SheetMetalConnectorData& data,
                              QString* errorMessage = nullptr);
};
