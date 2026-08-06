#pragma once

#include "CaeMsh2Reader.h"

#include <MeshVS_DataSource.hxx>
#include <TColStd_Array1OfReal.hxx>

namespace Cae {

class OccMeshDataSource;
DEFINE_STANDARD_HANDLE(OccMeshDataSource, MeshVS_DataSource)

class OccMeshDataSource final : public MeshVS_DataSource {
public:
    explicit OccMeshDataSource(Msh2MeshData meshData);

    Standard_Boolean GetGeom(
        Standard_Integer id,
        Standard_Boolean isElement,
        TColStd_Array1OfReal& coordinates,
        Standard_Integer& nodeCount,
        MeshVS_EntityType& type) const override;
    Standard_Boolean GetGeomType(
        Standard_Integer id,
        Standard_Boolean isElement,
        MeshVS_EntityType& type) const override;
    Standard_Address GetAddr(Standard_Integer id, Standard_Boolean isElement) const override;
    Standard_Boolean GetNodesByElement(
        Standard_Integer id,
        TColStd_Array1OfInteger& nodeIds,
        Standard_Integer& nodeCount) const override;
    const TColStd_PackedMapOfInteger& GetAllNodes() const override;
    const TColStd_PackedMapOfInteger& GetAllElements() const override;

    DEFINE_STANDARD_RTTIEXT(OccMeshDataSource, MeshVS_DataSource)

private:
    Msh2MeshData m_meshData;
    std::map<int, std::vector<int>> m_elements;
    TColStd_PackedMapOfInteger m_nodeIds;
    TColStd_PackedMapOfInteger m_elementIds;
};

} // namespace Cae
