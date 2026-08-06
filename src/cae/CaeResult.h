#pragma once

#include "CaeTypes.h"

#include <QString>
#include <array>
#include <map>
#include <optional>
#include <vector>

namespace Cae {

struct CaeResultProbe {
    int nodeId{0};
    double value{0.0};
    std::optional<std::array<double, 3>> displacement;
};

class CaeResultField {
public:
    CaeResultField(
        ResultFieldType type,
        QString unit,
        double minValue,
        double maxValue,
        QString source,
        std::map<int, double> nodalValues = {},
        std::map<int, std::array<double, 3>> nodalDisplacements = {});

    ResultFieldType type() const;
    QString unit() const;
    double minValue() const;
    double maxValue() const;
    QString source() const;
    const std::map<int, double>& nodalValues() const;
    const std::map<int, std::array<double, 3>>& nodalDisplacements() const;
    std::optional<CaeResultProbe> probeNode(int nodeId) const;

private:
    ResultFieldType m_type{ResultFieldType::Displacement};
    QString m_unit;
    double m_minValue{0.0};
    double m_maxValue{0.0};
    QString m_source;
    std::map<int, double> m_nodalValues;
    std::map<int, std::array<double, 3>> m_nodalDisplacements;
};

class CaeResult {
public:
    void addOrReplaceField(CaeResultField field);
    const CaeResultField* field(ResultFieldType type) const;
    const std::vector<CaeResultField>& fields() const;

private:
    std::vector<CaeResultField> m_fields;
};

} // namespace Cae
