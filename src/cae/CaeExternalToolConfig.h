#pragma once

#include "CaeExternalProcess.h"

#include <QString>
#include <QStringList>

namespace Cae {

enum class ExternalTool {
    Gmsh,
    CalculiX,
    GetDP
};

QString toDisplayString(ExternalTool tool);

class CaeExternalToolConfig {
public:
    QString executablePath(ExternalTool tool) const;
    void setExecutablePath(ExternalTool tool, const QString& path);
    bool hasExecutablePath(ExternalTool tool) const;

    QString workingDirectory() const;
    void setWorkingDirectory(const QString& workingDirectory);

    int timeoutMilliseconds() const;
    void setTimeoutMilliseconds(int timeoutMilliseconds);

    ExternalProcessRequest createProcessRequest(ExternalTool tool, const QStringList& arguments = QStringList()) const;

private:
    QString m_gmshExecutablePath;
    QString m_calculixExecutablePath;
    QString m_getDpExecutablePath;
    QString m_workingDirectory;
    int m_timeoutMilliseconds{30000};
};

} // namespace Cae
