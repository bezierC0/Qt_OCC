#include "CaeMesh.h"

#include <utility>

namespace Cae {

CaeMeshSetup::CaeMeshSetup(double globalSize, MeshElementOrder elementOrder)
    : m_globalSize(globalSize)
    , m_elementOrder(elementOrder)
{
}

double CaeMeshSetup::globalSize() const
{
    return m_globalSize;
}

MeshElementOrder CaeMeshSetup::elementOrder() const
{
    return m_elementOrder;
}

CaeMesh::CaeMesh(CaeMeshSetup setup, int nodeCount, int elementCount, QString source)
    : m_setup(std::move(setup))
    , m_nodeCount(nodeCount)
    , m_elementCount(elementCount)
    , m_source(std::move(source))
{
}

const CaeMeshSetup& CaeMesh::setup() const
{
    return m_setup;
}

int CaeMesh::nodeCount() const
{
    return m_nodeCount;
}

int CaeMesh::elementCount() const
{
    return m_elementCount;
}

QString CaeMesh::source() const
{
    return m_source;
}

QString toDisplayString(MeshElementOrder order)
{
    switch (order) {
    case MeshElementOrder::First:
        return QStringLiteral("First Order");
    case MeshElementOrder::Second:
        return QStringLiteral("Second Order");
    }
    return QStringLiteral("Unknown Order");
}

} // namespace Cae
