#include "CaeBoundaryCondition.h"

#include <utility>

namespace Cae {

CaeBoundaryCondition::CaeBoundaryCondition(QString name, QString targetName, BoundaryConditionType type)
    : m_name(std::move(name))
    , m_targetName(std::move(targetName))
    , m_type(type)
{
}

CaeBoundaryCondition::CaeBoundaryCondition(QString name, QString targetName, BoundaryConditionType type, double value, QString unit)
    : m_name(std::move(name))
    , m_targetName(std::move(targetName))
    , m_type(type)
    , m_value(value)
    , m_unit(std::move(unit))
{
}

QString CaeBoundaryCondition::name() const
{
    return m_name;
}

QString CaeBoundaryCondition::targetName() const
{
    return m_targetName;
}

BoundaryConditionType CaeBoundaryCondition::type() const
{
    return m_type;
}

double CaeBoundaryCondition::value() const
{
    return m_value;
}

QString CaeBoundaryCondition::unit() const
{
    return m_unit;
}

QString CaeBoundaryCondition::summary() const
{
    if (m_unit.isEmpty()) {
        return QStringLiteral("%1 -> %2").arg(toDisplayString(m_type), m_targetName);
    }

    return QStringLiteral("%1 %2 -> %3").arg(m_value).arg(m_unit, m_targetName);
}

QString toDisplayString(BoundaryConditionType type)
{
    switch (type) {
    case BoundaryConditionType::FixedSupport:
        return QStringLiteral("Fixed Support");
    case BoundaryConditionType::Force:
        return QStringLiteral("Force");
    }
    return QStringLiteral("Unknown Boundary Condition");
}

} // namespace Cae
