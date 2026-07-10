#include "CaeDummyServices.h"

namespace Cae {

QString DummyMeshGenerator::name() const
{
    return QStringLiteral("Dummy Mesh Generator");
}

bool DummyMeshGenerator::generate(const MeshRequest& request, MeshResult* result, QString* errorMessage)
{
    if (!result) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Mesh result output is null.");
        }
        return false;
    }

    result->meshFilePath = QStringLiteral("dummy://mesh/global-size-%1").arg(request.globalSize);
    result->nodeCount = 125;
    result->elementCount = 384;
    return true;
}

QString DummySolver::name() const
{
    return QStringLiteral("Dummy Solver");
}

bool DummySolver::solve(const SolverRequest& request, SolverResult* result, QString* errorMessage)
{
    Q_UNUSED(request)

    if (!result) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Solver result output is null.");
        }
        return false;
    }

    result->resultFilePath = QStringLiteral("dummy://solver/result");
    result->logFilePath = QStringLiteral("dummy://solver/log");
    return true;
}

QString DummyResultReader::name() const
{
    return QStringLiteral("Dummy Result Reader");
}

bool DummyResultReader::read(const SolverResult& solverResult, ResultField* field, QString* errorMessage)
{
    Q_UNUSED(solverResult)

    if (!field) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Result field output is null.");
        }
        return false;
    }

    switch (field->type) {
    case ResultFieldType::Displacement:
        field->values = {0.0, 0.018, 0.041, 0.067, 0.092};
        break;
    case ResultFieldType::VonMisesStress:
        field->values = {3.2, 18.4, 47.8, 82.5, 126.0};
        break;
    case ResultFieldType::Temperature:
        field->values = {20.0, 37.5, 55.0, 72.5, 90.0};
        break;
    }

    return true;
}

} // namespace Cae
