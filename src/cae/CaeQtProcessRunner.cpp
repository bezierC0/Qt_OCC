#include "CaeQtProcessRunner.h"

#include <QProcess>

namespace Cae {

bool QtProcessRunner::run(const ExternalProcessRequest& request, ExternalProcessResult* result, QString* errorMessage)
{
    if (!result) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("External process result output is null.");
        }
        return false;
    }

    if (request.program.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("External process program is empty.");
        }
        return false;
    }

    QProcess process;
    if (!request.workingDirectory.isEmpty()) {
        process.setWorkingDirectory(request.workingDirectory);
    }

    process.start(request.program, request.arguments);
    if (!process.waitForStarted()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to start external process: %1").arg(request.program);
        }
        return false;
    }

    if (!process.waitForFinished(request.timeoutMilliseconds)) {
        process.kill();
        process.waitForFinished();
        if (errorMessage) {
            *errorMessage = QStringLiteral("External process timed out: %1").arg(request.program);
        }
        return false;
    }

    result->exitCode = process.exitCode();
    result->standardOutput = QString::fromLocal8Bit(process.readAllStandardOutput());
    result->standardError = QString::fromLocal8Bit(process.readAllStandardError());

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("External process failed: %1").arg(request.program);
        }
        return false;
    }

    return true;
}

} // namespace Cae
