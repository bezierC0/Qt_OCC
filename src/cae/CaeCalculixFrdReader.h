#pragma once

#include "CaeServiceInterfaces.h"

namespace Cae {

class CalculixFrdReader final : public IResultReader {
public:
    QString name() const override;
    bool read(const SolverResult& solverResult, ResultField* field, QString* errorMessage) override;
};

} // namespace Cae
