#include "CaeCalculixSolver.h"

#include "CaeCalculixInputWriter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <utility>

namespace Cae {

CalculixSolver::CalculixSolver(CaeExternalToolConfig config, IExternalProcessRunner* processRunner)
    : m_config(std::move(config))
    , m_processRunner(processRunner)
{
}

QString CalculixSolver::name() const
{
    return QStringLiteral("CalculiX");
}

bool CalculixSolver::solve(const SolverRequest& request, SolverResult* result, QString* errorMessage)
{
    if (!result || !m_processRunner) {
        if (errorMessage) *errorMessage = QStringLiteral("CalculiX solver is not initialized.");
        return false;
    }
    if (!QFileInfo::exists(request.meshFilePath)) {
        if (errorMessage) *errorMessage = QStringLiteral("CalculiX mesh file does not exist: %1").arg(request.meshFilePath);
        return false;
    }
    if (request.studyType != StudyType::StaticStructural) {
        if (errorMessage) *errorMessage = QStringLiteral("Real CalculiX thermal input is not implemented yet.");
        return false;
    }

    QString workingDirectory = request.workingDirectory;
    if (workingDirectory.isEmpty()) workingDirectory = m_config.workingDirectory();
    if (workingDirectory.isEmpty()) workingDirectory = QFileInfo(request.meshFilePath).absolutePath();
    if (!QDir().mkpath(workingDirectory)) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to create CalculiX working directory: %1").arg(workingDirectory);
        return false;
    }

    const QString jobName = QStringLiteral("cae_job");
    const QString inputFilePath = QDir(workingDirectory).filePath(jobName + QStringLiteral(".inp"));
    const QString resultFilePath = QDir(workingDirectory).filePath(jobName + QStringLiteral(".frd"));
    const QString logFilePath = QDir(workingDirectory).filePath(jobName + QStringLiteral(".log"));
    QFile::remove(resultFilePath);
    QFile::remove(logFilePath);

    if (!CalculixInputWriter::writeStaticAnalysis(request, inputFilePath, errorMessage)) return false;

    ExternalProcessRequest processRequest = m_config.createProcessRequest(
        ExternalTool::CalculiX, {QStringLiteral("-i"), jobName});
    processRequest.workingDirectory = workingDirectory;
    ExternalProcessResult processResult;
    const bool processSucceeded = m_processRunner->run(processRequest, &processResult, errorMessage);

    QFile logFile(logFilePath);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream logStream(&logFile);
        logStream << processResult.standardOutput;
        if (!processResult.standardError.isEmpty()) logStream << "\n" << processResult.standardError;
    }

    if (!processSucceeded) {
        if (errorMessage && !processResult.standardError.trimmed().isEmpty()) {
            *errorMessage += QStringLiteral("\n%1").arg(processResult.standardError.trimmed());
        }
        return false;
    }
    if (!QFileInfo::exists(resultFilePath)) {
        if (errorMessage) *errorMessage = QStringLiteral("CalculiX completed without producing FRD output. See %1").arg(logFilePath);
        return false;
    }

    result->resultFilePath = resultFilePath;
    result->logFilePath = logFilePath;
    return true;
}

} // namespace Cae
