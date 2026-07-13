#include "CaeGmshMeshGenerator.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

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
    QFile meshFile(meshFilePath);
    if (!meshFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Gmsh did not produce a readable mesh file: %1").arg(meshFilePath);
        }
        return false;
    }

    QTextStream stream(&meshFile);
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line == QStringLiteral("$Nodes") && !stream.atEnd()) {
            result->nodeCount = stream.readLine().trimmed().toInt();
        } else if (line == QStringLiteral("$Elements") && !stream.atEnd()) {
            result->elementCount = stream.readLine().trimmed().toInt();
        }
    }

    if (result->nodeCount <= 0 || result->elementCount <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Gmsh mesh contains no nodes or elements: %1").arg(meshFilePath);
        }
        return false;
    }
    return true;
}

} // namespace Cae
