#pragma once

#include <QString>

namespace Cae {

enum class StudyType {
    StaticStructural,
    SteadyThermal
};

enum class StudyState {
    Empty,
    GeometryReady,
    Meshed,
    Solving,
    Solved,
    Failed
};

enum class ResultFieldType {
    Displacement,
    VonMisesStress,
    Temperature
};

QString toDisplayString(StudyType type);
QString toDisplayString(StudyState state);
QString toDisplayString(ResultFieldType type);

} // namespace Cae
