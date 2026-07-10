#include "CaeExternalToolConfig.h"

namespace Cae {

QString toDisplayString(ExternalTool tool)
{
    switch (tool) {
    case ExternalTool::Gmsh:
        return QStringLiteral("Gmsh");
    case ExternalTool::CalculiX:
        return QStringLiteral("CalculiX");
    case ExternalTool::GetDP:
        return QStringLiteral("GetDP");
    }

    return QStringLiteral("Unknown");
}

QString CaeExternalToolConfig::executablePath(ExternalTool tool) const
{
    switch (tool) {
    case ExternalTool::Gmsh:
        return m_gmshExecutablePath;
    case ExternalTool::CalculiX:
        return m_calculixExecutablePath;
    case ExternalTool::GetDP:
        return m_getDpExecutablePath;
    }

    return QString();
}

void CaeExternalToolConfig::setExecutablePath(ExternalTool tool, const QString& path)
{
    switch (tool) {
    case ExternalTool::Gmsh:
        m_gmshExecutablePath = path;
        break;
    case ExternalTool::CalculiX:
        m_calculixExecutablePath = path;
        break;
    case ExternalTool::GetDP:
        m_getDpExecutablePath = path;
        break;
    }
}

bool CaeExternalToolConfig::hasExecutablePath(ExternalTool tool) const
{
    return !executablePath(tool).isEmpty();
}

QString CaeExternalToolConfig::workingDirectory() const
{
    return m_workingDirectory;
}

void CaeExternalToolConfig::setWorkingDirectory(const QString& workingDirectory)
{
    m_workingDirectory = workingDirectory;
}

int CaeExternalToolConfig::timeoutMilliseconds() const
{
    return m_timeoutMilliseconds;
}

void CaeExternalToolConfig::setTimeoutMilliseconds(int timeoutMilliseconds)
{
    if (timeoutMilliseconds > 0) {
        m_timeoutMilliseconds = timeoutMilliseconds;
    }
}

ExternalProcessRequest CaeExternalToolConfig::createProcessRequest(ExternalTool tool, const QStringList& arguments) const
{
    ExternalProcessRequest request;
    request.program = executablePath(tool);
    request.arguments = arguments;
    request.workingDirectory = m_workingDirectory;
    request.timeoutMilliseconds = m_timeoutMilliseconds;
    return request;
}

} // namespace Cae
