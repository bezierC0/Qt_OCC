#include "CaeSolution.h"

#include <utility>

namespace Cae {

CaeSolverSetup::CaeSolverSetup(SolverBackend backend, StudyType studyType)
    : m_backend(backend)
    , m_studyType(studyType)
{
}

SolverBackend CaeSolverSetup::backend() const
{
    return m_backend;
}

StudyType CaeSolverSetup::studyType() const
{
    return m_studyType;
}

CaeSolution::CaeSolution(
    CaeSolverSetup setup,
    SolutionStatus status,
    QString summary,
    QString resultFilePath,
    QString logFilePath)
    : m_setup(std::move(setup))
    , m_status(status)
    , m_summary(std::move(summary))
    , m_resultFilePath(std::move(resultFilePath))
    , m_logFilePath(std::move(logFilePath))
{
}

const CaeSolverSetup& CaeSolution::setup() const
{
    return m_setup;
}

SolutionStatus CaeSolution::status() const
{
    return m_status;
}

QString CaeSolution::summary() const
{
    return m_summary;
}

QString CaeSolution::resultFilePath() const
{
    return m_resultFilePath;
}

QString CaeSolution::logFilePath() const
{
    return m_logFilePath;
}

QString toDisplayString(SolverBackend backend)
{
    switch (backend) {
    case SolverBackend::CalculiX:
        return QStringLiteral("CalculiX");
    case SolverBackend::GetDP:
        return QStringLiteral("GetDP");
    }
    return QStringLiteral("Unknown Solver");
}

QString toDisplayString(SolutionStatus status)
{
    switch (status) {
    case SolutionStatus::Prepared:
        return QStringLiteral("Prepared");
    case SolutionStatus::Solved:
        return QStringLiteral("Solved");
    case SolutionStatus::Failed:
        return QStringLiteral("Failed");
    }
    return QStringLiteral("Unknown Status");
}

} // namespace Cae
