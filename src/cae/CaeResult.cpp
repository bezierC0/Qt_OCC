#include "CaeResult.h"

#include <algorithm>
#include <utility>

namespace Cae {

CaeResultField::CaeResultField(
    ResultFieldType type,
    QString unit,
    double minValue,
    double maxValue,
    QString source,
    std::map<int, double> nodalValues,
    std::map<int, std::array<double, 3>> nodalDisplacements)
    : m_type(type)
    , m_unit(std::move(unit))
    , m_minValue(minValue)
    , m_maxValue(maxValue)
    , m_source(std::move(source))
    , m_nodalValues(std::move(nodalValues))
    , m_nodalDisplacements(std::move(nodalDisplacements))
{
}

ResultFieldType CaeResultField::type() const
{
    return m_type;
}

QString CaeResultField::unit() const
{
    return m_unit;
}

double CaeResultField::minValue() const
{
    return m_minValue;
}

double CaeResultField::maxValue() const
{
    return m_maxValue;
}

QString CaeResultField::source() const
{
    return m_source;
}

const std::map<int, double>& CaeResultField::nodalValues() const
{
    return m_nodalValues;
}

const std::map<int, std::array<double, 3>>& CaeResultField::nodalDisplacements() const
{
    return m_nodalDisplacements;
}

std::optional<CaeResultProbe> CaeResultField::probeNode(int nodeId) const
{
    const auto value = m_nodalValues.find(nodeId);
    if (value == m_nodalValues.end()) {
        return std::nullopt;
    }

    CaeResultProbe probe;
    probe.nodeId = nodeId;
    probe.value = value->second;
    const auto displacement = m_nodalDisplacements.find(nodeId);
    if (displacement != m_nodalDisplacements.end()) {
        probe.displacement = displacement->second;
    }
    return probe;
}

void CaeResult::addOrReplaceField(CaeResultField field)
{
    const auto existing = std::find_if(
        m_fields.begin(),
        m_fields.end(),
        [&field](const CaeResultField& item) {
            return item.type() == field.type();
        });

    if (existing != m_fields.end()) {
        *existing = std::move(field);
        return;
    }

    m_fields.push_back(std::move(field));
}

const CaeResultField* CaeResult::field(ResultFieldType type) const
{
    const auto result = std::find_if(
        m_fields.cbegin(),
        m_fields.cend(),
        [type](const CaeResultField& item) {
            return item.type() == type;
        });
    return result == m_fields.cend() ? nullptr : &(*result);
}

const std::vector<CaeResultField>& CaeResult::fields() const
{
    return m_fields;
}

} // namespace Cae
