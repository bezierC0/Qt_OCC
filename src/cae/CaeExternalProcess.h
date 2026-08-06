#pragma once

#include <QString>
#include <QStringList>

namespace Cae {

struct ExternalProcessRequest {
    QString program;
    QStringList arguments;
    QString workingDirectory;
    int timeoutMilliseconds{30000};
};

struct ExternalProcessResult {
    int exitCode{-1};
    QString standardOutput;
    QString standardError;
};

class IExternalProcessRunner {
public:
    virtual ~IExternalProcessRunner() = default;
    virtual bool run(const ExternalProcessRequest& request, ExternalProcessResult* result, QString* errorMessage) = 0;
};

} // namespace Cae
