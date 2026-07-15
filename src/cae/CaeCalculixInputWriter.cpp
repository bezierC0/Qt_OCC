#include "CaeCalculixInputWriter.h"

#include "CaeMsh2Reader.h"

#include <algorithm>
#include <cmath>

#include <QFile>
#include <QTextStream>

namespace Cae {

namespace {

void writeIdList(QTextStream& stream, const std::vector<int>& ids)
{
    constexpr int idsPerLine = 16;
    for (std::size_t index = 0; index < ids.size(); ++index) {
        stream << ids[index];
        const bool lineEnd = ((index + 1) % idsPerLine == 0) || index + 1 == ids.size();
        stream << (lineEnd ? "\n" : ", ");
    }
}

QString calculixElementType(int gmshType)
{
    switch (gmshType) {
    case 4:
        return QStringLiteral("C3D4");
    case 5:
        return QStringLiteral("C3D8");
    default:
        return QString();
    }
}

} // namespace

bool CalculixInputWriter::writeStaticAnalysis(
    const SolverRequest& request,
    const QString& inputFilePath,
    QString* errorMessage)
{
    Msh2MeshData meshData;
    if (!Msh2Reader::read(request.meshFilePath, &meshData, errorMessage)) {
        return false;
    }
    if (meshData.volumeElements.empty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("CalculiX requires tetrahedral or hexahedral volume elements.");
        }
        return false;
    }

    const auto minMaxX = std::minmax_element(
        meshData.nodes.cbegin(), meshData.nodes.cend(),
        [](const auto& left, const auto& right) { return left.second[0] < right.second[0]; });
    const double minimumX = minMaxX.first->second[0];
    const double maximumX = minMaxX.second->second[0];
    const double tolerance = std::max(1.0e-9, std::abs(maximumX - minimumX) * 1.0e-6);

    std::vector<int> fixedNodes;
    std::vector<int> loadNodes;
    for (const auto& node : meshData.nodes) {
        if (std::abs(node.second[0] - minimumX) <= tolerance) fixedNodes.push_back(node.first);
        if (std::abs(node.second[0] - maximumX) <= tolerance) loadNodes.push_back(node.first);
    }
    if (fixedNodes.empty() || loadNodes.empty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to create automatic fixed and load node sets.");
        return false;
    }

    QFile inputFile(inputFilePath);
    if (!inputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = QStringLiteral("Cannot write CalculiX input file: %1").arg(inputFilePath);
        return false;
    }

    QTextStream stream(&inputFile);
    stream << "*HEADING\nQt_OCC automatic static structural analysis\n*NODE\n";
    for (const auto& node : meshData.nodes) {
        stream << node.first << ", " << QString::number(node.second[0], 'g', 16) << ", "
               << QString::number(node.second[1], 'g', 16) << ", "
               << QString::number(node.second[2], 'g', 16) << "\n";
    }

    std::vector<int> allElements;
    for (int gmshType : {4, 5}) {
        bool headerWritten = false;
        for (const Msh2Element& element : meshData.volumeElements) {
            if (element.gmshType != gmshType) continue;
            if (!headerWritten) {
                stream << "*ELEMENT, TYPE=" << calculixElementType(gmshType)
                       << ", ELSET=EALL_" << gmshType << "\n";
                headerWritten = true;
            }
            stream << element.id;
            for (int nodeId : element.nodeIds) stream << ", " << nodeId;
            stream << "\n";
            allElements.push_back(element.id);
        }
    }
    if (allElements.empty()) {
        if (errorMessage) *errorMessage = QStringLiteral("No supported CalculiX volume elements were found.");
        return false;
    }

    stream << "*ELSET, ELSET=EALL\n";
    writeIdList(stream, allElements);
    stream << "*NSET, NSET=FIXED\n";
    writeIdList(stream, fixedNodes);
    stream << "*NSET, NSET=LOAD\n";
    writeIdList(stream, loadNodes);
    stream << "*MATERIAL, NAME=DEFAULT_STEEL\n*ELASTIC\n"
           << request.youngModulus << ", " << request.poissonRatio << "\n";
    stream << "*SOLID SECTION, ELSET=EALL, MATERIAL=DEFAULT_STEEL\n\n";
    stream << "*BOUNDARY\nFIXED, 1, 3, 0.0\n*STEP\n*STATIC\n";
    stream << "*CLOAD\nLOAD, 1, "
           << request.force / static_cast<double>(loadNodes.size()) << "\n";
    stream << "*NODE FILE\nU\n*EL FILE\nS\n*END STEP\n";
    return true;
}

} // namespace Cae
