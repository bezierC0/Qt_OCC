#pragma once

#include <QString>

namespace Cae {

enum class MeshElementOrder {
    First,
    Second
};

class CaeMeshSetup {
public:
    CaeMeshSetup(double globalSize, MeshElementOrder elementOrder);

    double globalSize() const;
    MeshElementOrder elementOrder() const;

private:
    double m_globalSize{1.0};
    MeshElementOrder m_elementOrder{MeshElementOrder::First};
};

class CaeMesh {
public:
    CaeMesh(
        CaeMeshSetup setup,
        int nodeCount,
        int surfaceElementCount,
        int volumeElementCount,
        QString source);

    const CaeMeshSetup& setup() const;
    int nodeCount() const;
    int elementCount() const;
    int surfaceElementCount() const;
    int volumeElementCount() const;
    QString source() const;

private:
    CaeMeshSetup m_setup;
    int m_nodeCount{0};
    int m_surfaceElementCount{0};
    int m_volumeElementCount{0};
    QString m_source;
};

QString toDisplayString(MeshElementOrder order);

} // namespace Cae
