#include "CaeExternalToolConfigStore.h"

#include <QSettings>

namespace {

constexpr const char* kOrganization = "Qt_OCC";
constexpr const char* kApplication = "Qt_OCC";
constexpr const char* kGroup = "CAE/ExternalTools";
constexpr const char* kGmshPath = "gmshExecutablePath";
constexpr const char* kCalculixPath = "calculixExecutablePath";
constexpr const char* kGetDpPath = "getDpExecutablePath";
constexpr const char* kWorkingDirectory = "workingDirectory";
constexpr const char* kTimeoutMilliseconds = "timeoutMilliseconds";

QSettings createSettings()
{
    return QSettings(QString::fromLatin1(kOrganization), QString::fromLatin1(kApplication));
}

} // namespace

namespace Cae {

CaeExternalToolConfig CaeExternalToolConfigStore::load() const
{
    QSettings settings = createSettings();
    settings.beginGroup(QString::fromLatin1(kGroup));

    CaeExternalToolConfig config;
    config.setExecutablePath(ExternalTool::Gmsh, settings.value(QString::fromLatin1(kGmshPath)).toString());
    config.setExecutablePath(ExternalTool::CalculiX, settings.value(QString::fromLatin1(kCalculixPath)).toString());
    config.setExecutablePath(ExternalTool::GetDP, settings.value(QString::fromLatin1(kGetDpPath)).toString());
    config.setWorkingDirectory(settings.value(QString::fromLatin1(kWorkingDirectory)).toString());
    config.setTimeoutMilliseconds(settings.value(QString::fromLatin1(kTimeoutMilliseconds), config.timeoutMilliseconds()).toInt());

    settings.endGroup();
    return config;
}

void CaeExternalToolConfigStore::save(const CaeExternalToolConfig& config) const
{
    QSettings settings = createSettings();
    settings.beginGroup(QString::fromLatin1(kGroup));

    settings.setValue(QString::fromLatin1(kGmshPath), config.executablePath(ExternalTool::Gmsh));
    settings.setValue(QString::fromLatin1(kCalculixPath), config.executablePath(ExternalTool::CalculiX));
    settings.setValue(QString::fromLatin1(kGetDpPath), config.executablePath(ExternalTool::GetDP));
    settings.setValue(QString::fromLatin1(kWorkingDirectory), config.workingDirectory());
    settings.setValue(QString::fromLatin1(kTimeoutMilliseconds), config.timeoutMilliseconds());

    settings.endGroup();
}

} // namespace Cae
