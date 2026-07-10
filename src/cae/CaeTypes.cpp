#include "CaeTypes.h"

namespace Cae {

QString toDisplayString(StudyType type)
{
    switch (type) {
    case StudyType::StaticStructural:
        return QStringLiteral("Static Structural");
    case StudyType::SteadyThermal:
        return QStringLiteral("Steady Thermal");
    }
    return QStringLiteral("Unknown Study");
}

QString toDisplayString(StudyState state)
{
    switch (state) {
    case StudyState::Empty:
        return QStringLiteral("Empty");
    case StudyState::GeometryReady:
        return QStringLiteral("Geometry Ready");
    case StudyState::Meshed:
        return QStringLiteral("Meshed");
    case StudyState::Solving:
        return QStringLiteral("Solving");
    case StudyState::Solved:
        return QStringLiteral("Solved");
    case StudyState::Failed:
        return QStringLiteral("Failed");
    }
    return QStringLiteral("Unknown State");
}

QString toDisplayString(ResultFieldType type)
{
    switch (type) {
    case ResultFieldType::Displacement:
        return QStringLiteral("Displacement");
    case ResultFieldType::VonMisesStress:
        return QStringLiteral("Von Mises Stress");
    case ResultFieldType::Temperature:
        return QStringLiteral("Temperature");
    }
    return QStringLiteral("Unknown Result");
}

} // namespace Cae
