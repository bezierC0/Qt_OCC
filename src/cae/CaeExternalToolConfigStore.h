#pragma once

#include "CaeExternalToolConfig.h"

namespace Cae {

class CaeExternalToolConfigStore {
public:
    CaeExternalToolConfig load() const;
    void save(const CaeExternalToolConfig& config) const;
};

} // namespace Cae
