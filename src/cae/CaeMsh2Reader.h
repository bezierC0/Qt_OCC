#pragma once

#include <array>
#include <map>
#include <vector>

#include <QString>

namespace Cae {

struct Msh2Element {
    int id{0};
    int gmshType{0};
    std::vector<int> nodeIds;
};

struct Msh2MeshData {
    std::map<int, std::array<double, 3>> nodes;
    std::vector<Msh2Element> surfaceElements;
    std::vector<Msh2Element> volumeElements;
};

class Msh2Reader {
public:
    static bool read(const QString& filePath, Msh2MeshData* meshData, QString* errorMessage);
};

} // namespace Cae
