#include "CaeGmshMeshGenerator.h"

#include "CaeMsh2Reader.h"

#include <QDir>
#include <QFileInfo>

#include <utility>

namespace Cae {

GmshMeshGenerator::GmshMeshGenerator(CaeExternalToolConfig config, IExternalProcessRunner* processRunner)
    : m_config(std::move(config))
    , m_processRunner(processRunner)
{
}

QString GmshMeshGenerator::name() const
{
    return QStringLiteral("Gmsh");
}

bool GmshMeshGenerator::generate(const MeshRequest& request, MeshResult* result, QString* errorMessage)
{
    if (!result || !m_processRunner) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Gmsh mesh generator is not initialized.");
        }
        return false;
    }

    const QFileInfo geometryInfo(request.geometryFilePath);
    if (!geometryInfo.isFile()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("CAE geometry file does not exist: %1").arg(request.geometryFilePath);
        }
        return false;
    }

    QString workingDirectory = m_config.workingDirectory();
    if (workingDirectory.isEmpty()) {
        workingDirectory = geometryInfo.absolutePath();
    }
    if (!QDir().mkpath(workingDirectory)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to create CAE working directory: %1").arg(workingDirectory);
        }
        return false;
    }

    const QString meshFilePath = QDir(workingDirectory).filePath(QStringLiteral("cae_mesh.msh"));
    QFile::remove(meshFilePath);
    const QStringList arguments{
        geometryInfo.absoluteFilePath(),
        QStringLiteral("-3"),
        QStringLiteral("-format"),
        QStringLiteral("msh2"),
        QStringLiteral("-save_all"),
        QStringLiteral("-clmax"),
        QString::number(request.globalSize, 'g', 12),
        QStringLiteral("-o"),
        meshFilePath
    };

    ExternalProcessRequest processRequest = m_config.createProcessRequest(ExternalTool::Gmsh, arguments);
    processRequest.workingDirectory = workingDirectory;
    ExternalProcessResult processResult;
    if (!m_processRunner->run(processRequest, &processResult, errorMessage)) {
        if (errorMessage && !processResult.standardError.trimmed().isEmpty()) {
            *errorMessage += QStringLiteral("\n%1").arg(processResult.standardError.trimmed());
        }
        return false;
    }

    result->meshFilePath = meshFilePath;
    return readMeshStatistics(meshFilePath, result, errorMessage);
}

bool GmshMeshGenerator::readMeshStatistics(
    const QString& meshFilePath,
    MeshResult* result,
    QString* errorMessage) const
{
    Msh2MeshData meshData;
    if (!Msh2Reader::read(meshFilePath, &meshData, errorMessage)) {
        return false;
    }

    if (meshData.volumeElements.empty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Gmsh mesh contains no supported volume elements: %1").arg(meshFilePath);
        }
        return false;
    }

    result->nodeCount = static_cast<int>(meshData.nodes.size());
    result->surfaceElementCount = static_cast<int>(meshData.surfaceElements.size());
    result->volumeElementCount = static_cast<int>(meshData.volumeElements.size());
    return true;
}

} // namespace Cae
