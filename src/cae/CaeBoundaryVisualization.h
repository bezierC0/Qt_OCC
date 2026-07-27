#pragma once

#include "CaeNamedSelection.h"

#include <QString>
#include <array>
#include <vector>

namespace Cae {

enum class BoundaryMarkerType {
    FixedSupport,
    Force,
    Pressure
};

struct BoundaryMarker {
    BoundaryMarkerType type{BoundaryMarkerType::FixedSupport};
    QString label;
    PlanarSelectionRegion region;
    std::array<double, 3> direction{};
};

using BoundaryMarkers = std::vector<BoundaryMarker>;

} // namespace Cae
