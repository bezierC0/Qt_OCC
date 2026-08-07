#pragma once

#include "CaeServiceInterfaces.h"

namespace Cae {

class CalculixInputWriter {
public:
    static bool writeStaticAnalysis(
        const SolverRequest& request,
        const QString& inputFilePath,
        QString* errorMessage);
    static bool writeSteadyThermalAnalysis(
        const SolverRequest& request,
        const QString& inputFilePath,
        QString* errorMessage);
};

} // namespace Cae
