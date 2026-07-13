#pragma once

#include "CaeTypes.h"

#include <QString>

namespace Cae {

enum class SolverBackend {
    CalculiX,
    GetDP
};

enum class SolutionStatus {
    Prepared,
    Solved,
    Failed
};

class CaeSolverSetup {
public:
    CaeSolverSetup(SolverBackend backend, StudyType studyType);

    SolverBackend backend() const;
    StudyType studyType() const;

private:
    SolverBackend m_backend{SolverBackend::CalculiX};
    StudyType m_studyType{StudyType::StaticStructural};
};

class CaeSolution {
public:
    CaeSolution(
        CaeSolverSetup setup,
        SolutionStatus status,
        QString summary,
        QString resultFilePath = QString(),
        QString logFilePath = QString());

    const CaeSolverSetup& setup() const;
    SolutionStatus status() const;
    QString summary() const;
    QString resultFilePath() const;
    QString logFilePath() const;

private:
    CaeSolverSetup m_setup;
    SolutionStatus m_status{SolutionStatus::Prepared};
    QString m_summary;
    QString m_resultFilePath;
    QString m_logFilePath;
};

QString toDisplayString(SolverBackend backend);
QString toDisplayString(SolutionStatus status);

} // namespace Cae
