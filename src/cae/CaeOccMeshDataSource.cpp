#include "CaeOccMeshDataSource.h"

#include <utility>

namespace Cae {

IMPLEMENT_STANDARD_RTTIEXT(OccMeshDataSource, MeshVS_DataSource)

OccMeshDataSource::OccMeshDataSource(Msh2MeshData meshData)
    : m_meshData(std::move(meshData))
{
    for (const auto& node : m_meshData.nodes) {
        m_nodeIds.Add(node.first);
    }
    for (const Msh2Element& element : m_meshData.surfaceElements) {
        m_elements.emplace(element.id, element.nodeIds);
        m_elementIds.Add(element.id);
    }
}

Standard_Boolean OccMeshDataSource::GetGeom(
    Standard_Integer id,
    Standard_Boolean isElement,
    TColStd_Array1OfReal& coordinates,
    Standard_Integer& nodeCount,
    MeshVS_EntityType& type) const
{
    if (!isElement) {
        const auto node = m_meshData.nodes.find(id);
        if (node == m_meshData.nodes.end() || coordinates.Length() < 3) {
            return Standard_False;
        }
        nodeCount = 1;
        type = MeshVS_ET_Node;
        for (int coordinate = 0; coordinate < 3; ++coordinate) {
            coordinates.SetValue(coordinates.Lower() + coordinate, node->second[coordinate]);
        }
        return Standard_True;
    }

    const auto element = m_elements.find(id);
    if (element == m_elements.end()) {
        return Standard_False;
    }
    nodeCount = static_cast<Standard_Integer>(element->second.size());
    if (coordinates.Length() < nodeCount * 3) {
        return Standard_False;
    }
    type = MeshVS_ET_Face;
    int coordinateIndex = 0;
    for (int nodeId : element->second) {
        const auto node = m_meshData.nodes.find(nodeId);
        if (node == m_meshData.nodes.end()) {
            return Standard_False;
        }
        for (double value : node->second) {
            coordinates.SetValue(coordinates.Lower() + coordinateIndex++, value);
        }
    }
    return Standard_True;
}

Standard_Boolean OccMeshDataSource::GetGeomType(
    Standard_Integer id,
    Standard_Boolean isElement,
    MeshVS_EntityType& type) const
{
    if (isElement) {
        if (m_elements.find(id) == m_elements.end()) {
            return Standard_False;
        }
        type = MeshVS_ET_Face;
        return Standard_True;
    }
    if (m_meshData.nodes.find(id) == m_meshData.nodes.end()) {
        return Standard_False;
    }
    type = MeshVS_ET_Node;
    return Standard_True;
}

Standard_Address OccMeshDataSource::GetAddr(Standard_Integer, Standard_Boolean) const
{
    return nullptr;
}

Standard_Boolean OccMeshDataSource::GetNodesByElement(
    Standard_Integer id,
    TColStd_Array1OfInteger& nodeIds,
    Standard_Integer& nodeCount) const
{
    const auto element = m_elements.find(id);
    if (element == m_elements.end()) {
        return Standard_False;
    }
    nodeCount = static_cast<Standard_Integer>(element->second.size());
    if (nodeIds.Length() < nodeCount) {
        return Standard_False;
    }
    for (int index = 0; index < nodeCount; ++index) {
        nodeIds.SetValue(nodeIds.Lower() + index, element->second[static_cast<std::size_t>(index)]);
    }
    return Standard_True;
}

const TColStd_PackedMapOfInteger& OccMeshDataSource::GetAllNodes() const
{
    return m_nodeIds;
}

const TColStd_PackedMapOfInteger& OccMeshDataSource::GetAllElements() const
{
    return m_elementIds;
}

} // namespace Cae
