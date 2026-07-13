#include "CaeMsh2Reader.h"

#include <QFile>
#include <QTextStream>

#include <utility>

namespace Cae {

namespace {

QStringList fields(const QString& line)
{
    return line.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

int surfaceNodeCount(int gmshElementType)
{
    switch (gmshElementType) {
    case 2:
        return 3;
    case 3:
        return 4;
    default:
        return 0;
    }
}

} // namespace

bool Msh2Reader::read(const QString& filePath, Msh2MeshData* meshData, QString* errorMessage)
{
    if (!meshData) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("MSH2 output is null.");
        }
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot open Gmsh mesh: %1").arg(filePath);
        }
        return false;
    }

    meshData->nodes.clear();
    meshData->surfaceElements.clear();
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString section = stream.readLine().trimmed();
        if (section == QStringLiteral("$Nodes") && !stream.atEnd()) {
            const int nodeCount = stream.readLine().trimmed().toInt();
            for (int index = 0; index < nodeCount && !stream.atEnd(); ++index) {
                const QStringList values = fields(stream.readLine());
                if (values.size() < 4) {
                    continue;
                }
                meshData->nodes[values[0].toInt()] = {
                    values[1].toDouble(), values[2].toDouble(), values[3].toDouble()};
            }
        } else if (section == QStringLiteral("$Elements") && !stream.atEnd()) {
            const int elementCount = stream.readLine().trimmed().toInt();
            for (int index = 0; index < elementCount && !stream.atEnd(); ++index) {
                const QStringList values = fields(stream.readLine());
                if (values.size() < 4) {
                    continue;
                }

                const int nodeCount = surfaceNodeCount(values[1].toInt());
                const int tagCount = values[2].toInt();
                const int nodeOffset = 3 + tagCount;
                if (nodeCount == 0 || values.size() < nodeOffset + nodeCount) {
                    continue;
                }

                Msh2Element element;
                element.id = values[0].toInt();
                element.nodeIds.reserve(static_cast<std::size_t>(nodeCount));
                for (int nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
                    element.nodeIds.push_back(values[nodeOffset + nodeIndex].toInt());
                }
                meshData->surfaceElements.push_back(std::move(element));
            }
        }
    }

    if (meshData->nodes.empty() || meshData->surfaceElements.empty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("MSH2 contains no displayable surface elements: %1").arg(filePath);
        }
        return false;
    }
    return true;
}

} // namespace Cae
