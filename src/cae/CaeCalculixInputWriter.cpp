#include "CaeCalculixInputWriter.h"

#include "CaeMsh2Reader.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

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

QString calculixThermalElementType(int gmshType)
{
    switch (gmshType) {
    case 4:
        return QStringLiteral("DC3D4");
    case 5:
        return QStringLiteral("DC3D8");
    default:
        return QString();
    }
}

double meshTolerance(const Msh2MeshData& meshData)
{
    std::array<double, 3> meshMinimum = meshData.nodes.cbegin()->second;
    std::array<double, 3> meshMaximum = meshMinimum;
    for (const auto& node : meshData.nodes) {
        for (int axis = 0; axis < 3; ++axis) {
            meshMinimum[axis] = std::min(meshMinimum[axis], node.second[axis]);
            meshMaximum[axis] = std::max(meshMaximum[axis], node.second[axis]);
        }
    }
    const double dx = meshMaximum[0] - meshMinimum[0];
    const double dy = meshMaximum[1] - meshMinimum[1];
    const double dz = meshMaximum[2] - meshMinimum[2];
    return std::max(1.0e-7, std::sqrt(dx * dx + dy * dy + dz * dz) * 1.0e-6);
}

bool pointInRegion(
    const std::array<double, 3>& point,
    const PlanarSelectionRegion& region,
    double tolerance)
{
    const double planeDistance = std::abs(
        (point[0] - region.origin[0]) * region.normal[0] +
        (point[1] - region.origin[1]) * region.normal[1] +
        (point[2] - region.origin[2]) * region.normal[2]);
    if (planeDistance > tolerance) {
        return false;
    }
    for (int axis = 0; axis < 3; ++axis) {
        if (point[axis] < region.minimum[axis] - tolerance ||
            point[axis] > region.maximum[axis] + tolerance) {
            return false;
        }
    }
    return true;
}

std::vector<int> nodesInRegion(
    const Msh2MeshData& meshData,
    const PlanarSelectionRegion& region)
{
    const double tolerance = meshTolerance(meshData);
    std::vector<int> nodeIds;
    for (const auto& node : meshData.nodes) {
        if (pointInRegion(node.second, region, tolerance)) {
            nodeIds.push_back(node.first);
        }
    }
    return nodeIds;
}

double triangleArea(
    const std::array<double, 3>& a,
    const std::array<double, 3>& b,
    const std::array<double, 3>& c)
{
    const std::array<double, 3> ab{
        b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const std::array<double, 3> ac{
        c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    const std::array<double, 3> cross{
        ab[1] * ac[2] - ab[2] * ac[1],
        ab[2] * ac[0] - ab[0] * ac[2],
        ab[0] * ac[1] - ab[1] * ac[0]};
    return 0.5 * std::sqrt(
        cross[0] * cross[0] +
        cross[1] * cross[1] +
        cross[2] * cross[2]);
}

double surfaceElementArea(
    const Msh2Element& element,
    const Msh2MeshData& meshData)
{
    if (element.nodeIds.size() < 3) {
        return 0.0;
    }
    const auto& a = meshData.nodes.at(element.nodeIds[0]);
    const auto& b = meshData.nodes.at(element.nodeIds[1]);
    const auto& c = meshData.nodes.at(element.nodeIds[2]);
    double area = triangleArea(a, b, c);
    if (element.nodeIds.size() == 4) {
        const auto& d = meshData.nodes.at(element.nodeIds[3]);
        area += triangleArea(a, c, d);
    }
    return area;
}

std::map<int, std::array<double, 3>> pressureNodalLoads(
    const Msh2MeshData& meshData,
    const PlanarSelectionRegion& region,
    double pressure,
    double* loadedArea)
{
    const double tolerance = meshTolerance(meshData);
    std::map<int, std::array<double, 3>> nodalLoads;
    double areaSum = 0.0;
    for (const Msh2Element& element : meshData.surfaceElements) {
        bool isOnSelectedFace = !element.nodeIds.empty();
        for (int nodeId : element.nodeIds) {
            const auto node = meshData.nodes.find(nodeId);
            isOnSelectedFace = isOnSelectedFace &&
                node != meshData.nodes.end() &&
                pointInRegion(node->second, region, tolerance);
        }
        if (!isOnSelectedFace) {
            continue;
        }

        const double area = surfaceElementArea(element, meshData);
        areaSum += area;
        const double nodalMagnitude =
            pressure * area / static_cast<double>(element.nodeIds.size());
        for (int nodeId : element.nodeIds) {
            auto& load = nodalLoads[nodeId];
            for (int axis = 0; axis < 3; ++axis) {
                load[axis] -= region.normal[axis] * nodalMagnitude;
            }
        }
    }
    if (loadedArea) {
        *loadedArea = areaSum;
    }
    return nodalLoads;
}

std::map<int, double> heatFluxNodalLoads(
    const Msh2MeshData& meshData,
    const PlanarSelectionRegion& region,
    double heatFlux,
    double* loadedArea)
{
    const double tolerance = meshTolerance(meshData);
    std::map<int, double> nodalLoads;
    double areaSum = 0.0;
    for (const Msh2Element& element : meshData.surfaceElements) {
        bool isOnSelectedFace = !element.nodeIds.empty();
        for (int nodeId : element.nodeIds) {
            const auto node = meshData.nodes.find(nodeId);
            isOnSelectedFace = isOnSelectedFace &&
                node != meshData.nodes.end() &&
                pointInRegion(node->second, region, tolerance);
        }
        if (!isOnSelectedFace) {
            continue;
        }

        const double area = surfaceElementArea(element, meshData);
        areaSum += area;
        const double nodalLoad =
            heatFlux * area / static_cast<double>(element.nodeIds.size());
        for (int nodeId : element.nodeIds) {
            nodalLoads[nodeId] += nodalLoad;
        }
    }
    if (loadedArea) {
        *loadedArea = areaSum;
    }
    return nodalLoads;
}

struct CalculixElementFace {
    int elementId{0};
    int faceNumber{0};
};

std::vector<std::vector<int>> localFaces(int gmshType)
{
    if (gmshType == 4) {
        return {{0, 1, 2}, {0, 3, 1}, {1, 3, 2}, {2, 3, 0}};
    }
    if (gmshType == 5) {
        return {
            {0, 1, 2, 3},
            {4, 7, 6, 5},
            {0, 4, 5, 1},
            {1, 5, 6, 2},
            {2, 6, 7, 3},
            {3, 7, 4, 0}};
    }
    return {};
}

std::vector<int> sortedNodeIds(const std::vector<int>& nodeIds)
{
    std::vector<int> sorted = nodeIds;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

std::vector<CalculixElementFace> elementFacesInRegion(
    const Msh2MeshData& meshData,
    const PlanarSelectionRegion& region)
{
    const double tolerance = meshTolerance(meshData);
    std::set<std::vector<int>> selectedSurfaceFaces;
    for (const Msh2Element& surfaceElement : meshData.surfaceElements) {
        bool isOnSelectedFace = !surfaceElement.nodeIds.empty();
        for (int nodeId : surfaceElement.nodeIds) {
            const auto node = meshData.nodes.find(nodeId);
            isOnSelectedFace = isOnSelectedFace &&
                node != meshData.nodes.end() &&
                pointInRegion(node->second, region, tolerance);
        }
        if (isOnSelectedFace) {
            selectedSurfaceFaces.insert(sortedNodeIds(surfaceElement.nodeIds));
        }
    }

    std::vector<CalculixElementFace> elementFaces;
    for (const Msh2Element& volumeElement : meshData.volumeElements) {
        const auto faceDefinitions = localFaces(volumeElement.gmshType);
        for (std::size_t faceIndex = 0; faceIndex < faceDefinitions.size(); ++faceIndex) {
            std::vector<int> faceNodeIds;
            faceNodeIds.reserve(faceDefinitions[faceIndex].size());
            for (int localNodeIndex : faceDefinitions[faceIndex]) {
                faceNodeIds.push_back(volumeElement.nodeIds[localNodeIndex]);
            }
            if (selectedSurfaceFaces.count(sortedNodeIds(faceNodeIds)) > 0) {
                elementFaces.push_back({volumeElement.id, static_cast<int>(faceIndex + 1)});
            }
        }
    }
    return elementFaces;
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

    const bool hasForce =
        request.loadRegion.has_value() &&
        (request.force[0] != 0.0 || request.force[1] != 0.0 || request.force[2] != 0.0);
    const bool hasPressure =
        request.pressureRegion.has_value() && request.pressure > 0.0;
    if (!request.fixedRegion || (!hasForce && !hasPressure)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "CalculiX analysis requires a Fixed Support and at least one planar surface load.");
        }
        return false;
    }

    const std::vector<int> fixedNodes = nodesInRegion(meshData, *request.fixedRegion);
    const std::vector<int> loadNodes = hasForce
        ? nodesInRegion(meshData, *request.loadRegion)
        : std::vector<int>{};
    if (fixedNodes.empty() || (hasForce && loadNodes.empty())) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Failed to map the selected CAD faces to mesh nodes (fixed=%1, load=%2).")
                .arg(fixedNodes.size())
                .arg(loadNodes.size());
        }
        return false;
    }

    std::map<int, std::array<double, 3>> nodalLoads;
    if (hasForce) {
        for (int nodeId : loadNodes) {
            auto& load = nodalLoads[nodeId];
            for (int axis = 0; axis < 3; ++axis) {
                load[axis] += request.force[axis] / static_cast<double>(loadNodes.size());
            }
        }
    }
    if (hasPressure) {
        double loadedArea = 0.0;
        const auto pressureLoads = pressureNodalLoads(
            meshData,
            *request.pressureRegion,
            request.pressure,
            &loadedArea);
        if (pressureLoads.empty() || loadedArea <= 0.0) {
            if (errorMessage) {
                *errorMessage = QStringLiteral(
                    "Failed to map the pressure face to surface mesh elements.");
            }
            return false;
        }
        for (const auto& nodeLoad : pressureLoads) {
            auto& load = nodalLoads[nodeLoad.first];
            for (int axis = 0; axis < 3; ++axis) {
                load[axis] += nodeLoad.second[axis];
            }
        }
    }

    QFile inputFile(inputFilePath);
    if (!inputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = QStringLiteral("Cannot write CalculiX input file: %1").arg(inputFilePath);
        return false;
    }

    QTextStream stream(&inputFile);
    stream << "*HEADING\nQt_OCC planar-face static structural analysis\n*NODE\n";
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
    stream << "*MATERIAL, NAME=DEFAULT_STEEL\n*ELASTIC\n"
           << request.youngModulus << ", " << request.poissonRatio << "\n";
    stream << "*SOLID SECTION, ELSET=EALL, MATERIAL=DEFAULT_STEEL\n\n";
    stream << "*BOUNDARY\nFIXED, 1, 3, 0.0\n*STEP\n*STATIC\n";
    stream << "*CLOAD\n";
    for (const auto& nodeLoad : nodalLoads) {
        for (int axis = 0; axis < 3; ++axis) {
            if (nodeLoad.second[axis] != 0.0) {
                stream << nodeLoad.first << ", " << axis + 1 << ", "
                       << nodeLoad.second[axis] << "\n";
            }
        }
    }
    stream << "*NODE FILE\nU\n*EL FILE\nS\n*END STEP\n";
    return true;
}

bool CalculixInputWriter::writeSteadyThermalAnalysis(
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
    const bool hasHeatFlux = request.heatFluxRegion && request.heatFlux > 0.0;
    const bool hasConvection = request.convectionRegion && request.filmCoefficient > 0.0;
    if (!request.fixedTemperatureRegion || request.thermalConductivity <= 0.0 ||
        (!hasHeatFlux && !hasConvection)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Steady thermal analysis requires conductivity, a fixed temperature and a thermal load.");
        }
        return false;
    }

    const std::vector<int> fixedNodes =
        nodesInRegion(meshData, *request.fixedTemperatureRegion);
    double loadedArea = 0.0;
    const std::map<int, double> nodalHeatLoads = hasHeatFlux
        ? heatFluxNodalLoads(
              meshData,
              *request.heatFluxRegion,
              request.heatFlux,
              &loadedArea)
        : std::map<int, double>{};
    const std::vector<CalculixElementFace> convectionFaces = hasConvection
        ? elementFacesInRegion(meshData, *request.convectionRegion)
        : std::vector<CalculixElementFace>{};
    if (fixedNodes.empty() ||
        (hasHeatFlux && (nodalHeatLoads.empty() || loadedArea <= 0.0)) ||
        (hasConvection && convectionFaces.empty())) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Failed to map thermal faces (fixed=%1, heat-flux nodes=%2, convection faces=%3).")
                .arg(fixedNodes.size())
                .arg(nodalHeatLoads.size())
                .arg(convectionFaces.size());
        }
        return false;
    }

    QFile inputFile(inputFilePath);
    if (!inputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot write CalculiX input file: %1").arg(inputFilePath);
        }
        return false;
    }

    QTextStream stream(&inputFile);
    stream << "*HEADING\nQt_OCC planar-face steady thermal analysis\n*NODE\n";
    for (const auto& node : meshData.nodes) {
        stream << node.first << ", " << QString::number(node.second[0], 'g', 16) << ", "
               << QString::number(node.second[1], 'g', 16) << ", "
               << QString::number(node.second[2], 'g', 16) << "\n";
    }

    std::vector<int> allElements;
    for (int gmshType : {4, 5}) {
        bool headerWritten = false;
        for (const Msh2Element& element : meshData.volumeElements) {
            if (element.gmshType != gmshType) {
                continue;
            }
            if (!headerWritten) {
                stream << "*ELEMENT, TYPE=" << calculixThermalElementType(gmshType)
                       << ", ELSET=EALL_" << gmshType << "\n";
                headerWritten = true;
            }
            stream << element.id;
            for (int nodeId : element.nodeIds) {
                stream << ", " << nodeId;
            }
            stream << "\n";
            allElements.push_back(element.id);
        }
    }
    if (allElements.empty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No supported CalculiX thermal volume elements were found.");
        }
        return false;
    }

    stream << "*ELSET, ELSET=EALL\n";
    writeIdList(stream, allElements);
    stream << "*NSET, NSET=FIXED_TEMP\n";
    writeIdList(stream, fixedNodes);
    stream << "*MATERIAL, NAME=THERMAL_MATERIAL\n*CONDUCTIVITY\n"
           << QString::number(request.thermalConductivity / 1000.0, 'g', 16) << "\n";
    stream << "*SOLID SECTION, ELSET=EALL, MATERIAL=THERMAL_MATERIAL\n\n";
    stream << "*STEP\n*HEAT TRANSFER, STEADY STATE\n1., 1.\n";
    stream << "*BOUNDARY\nFIXED_TEMP, 11, 11, "
           << QString::number(request.fixedTemperature, 'g', 16) << "\n";
    if (hasHeatFlux) {
        stream << "*CFLUX\n";
        for (const auto& nodeLoad : nodalHeatLoads) {
            stream << nodeLoad.first << ", 11, "
                   << QString::number(nodeLoad.second, 'g', 16) << "\n";
        }
    }
    if (hasConvection) {
        stream << "*FILM\n";
        for (const CalculixElementFace& face : convectionFaces) {
            stream << face.elementId << ", F" << face.faceNumber << ", "
                   << QString::number(request.ambientTemperature, 'g', 16) << ", "
                   << QString::number(request.filmCoefficient / 1.0e6, 'g', 16) << "\n";
        }
    }
    stream << "*NODE FILE\nNT\n*END STEP\n";
    return true;
}

} // namespace Cae
