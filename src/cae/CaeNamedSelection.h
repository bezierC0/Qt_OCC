#pragma once

#include <QString>
#include <array>
#include <optional>

namespace Cae {

enum class NamedSelectionScope {
    Geometry,
    Face,
    Edge,
    Vertex
};

struct PlanarSelectionRegion {
    std::array<double, 3> origin{};
    std::array<double, 3> normal{};
    std::array<double, 3> minimum{};
    std::array<double, 3> maximum{};
};

class CaeNamedSelection {
public:
    CaeNamedSelection(QString name, NamedSelectionScope scope);
    CaeNamedSelection(QString name, NamedSelectionScope scope, PlanarSelectionRegion region);

    QString name() const;
    NamedSelectionScope scope() const;
    const std::optional<PlanarSelectionRegion>& planarRegion() const;

private:
    QString m_name;
    NamedSelectionScope m_scope{NamedSelectionScope::Geometry};
    std::optional<PlanarSelectionRegion> m_planarRegion;
};

QString toDisplayString(NamedSelectionScope scope);

} // namespace Cae
