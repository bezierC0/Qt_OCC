#pragma once

#include "CaeServiceInterfaces.h"

namespace Cae {

class DummyMeshGenerator final : public IMeshGenerator {
public:
    QString name() const override;
    bool generate(const MeshRequest& request, MeshResult* result, QString* errorMessage) override;
};

class DummySolver final : public ISolver {
public:
    QString name() const override;
    bool solve(const SolverRequest& request, SolverResult* result, QString* errorMessage) override;
};

class DummyResultReader final : public IResultReader {
public:
    QString name() const override;
    bool read(const SolverResult& solverResult, ResultField* field, QString* errorMessage) override;
};

} // namespace Cae
