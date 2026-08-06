#pragma once

#include "CaeExternalProcess.h"
#include "CaeExternalToolConfig.h"
#include "CaeServiceInterfaces.h"

namespace Cae {

class CalculixSolver final : public ISolver {
public:
    CalculixSolver(CaeExternalToolConfig config, IExternalProcessRunner* processRunner);

    QString name() const override;
    bool solve(const SolverRequest& request, SolverResult* result, QString* errorMessage) override;

private:
    CaeExternalToolConfig m_config;
    IExternalProcessRunner* m_processRunner{nullptr};
};

} // namespace Cae
