#pragma once

#include "CaeExternalProcess.h"
#include "CaeExternalToolConfig.h"
#include "CaeServiceInterfaces.h"

#include <memory>

namespace Cae {

enum class CaeServiceProfile {
    Dummy
};

QString toDisplayString(CaeServiceProfile profile);

struct CaeServiceBundle {
    CaeServiceProfile profile{CaeServiceProfile::Dummy};
    std::unique_ptr<IMeshGenerator> meshGenerator;
    std::unique_ptr<ISolver> solver;
    std::unique_ptr<IResultReader> resultReader;
    std::unique_ptr<IExternalProcessRunner> processRunner;
    CaeExternalToolConfig externalToolConfig;

    bool isValid() const;
};

class CaeServiceFactory {
public:
    static CaeServiceBundle create(CaeServiceProfile profile = CaeServiceProfile::Dummy);
};

} // namespace Cae
