#pragma once

#include "CaeExternalProcess.h"

namespace Cae {

class QtProcessRunner final : public IExternalProcessRunner {
public:
    bool run(const ExternalProcessRequest& request, ExternalProcessResult* result, QString* errorMessage) override;
};

} // namespace Cae
