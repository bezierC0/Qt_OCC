#include "CaeResult.h"

#include <algorithm>
#include <utility>

namespace Cae {

CaeResultField::CaeResultField(ResultFieldType type, QString unit, double minValue, double maxValue, QString source)
    : m_type(type)
    , m_unit(std::move(unit))
    , m_minValue(minValue)
    , m_maxValue(maxValue)
    , m_source(std::move(source))
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

void CaeResult::addOrReplaceField(const CaeResultField& field)
{
    const auto existing = std::find_if(
        m_fields.begin(),
        m_fields.end(),
        [&field](const CaeResultField& item) {
            return item.type() == field.type();
        });

    if (existing != m_fields.end()) {
        *existing = field;
        return;
    }

    m_fields.push_back(field);
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
