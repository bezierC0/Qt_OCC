#include "CaeMaterial.h"

#include <utility>

namespace Cae {

CaeMaterial::CaeMaterial(QString name, QString targetName, double youngModulus, double poissonRatio)
    : m_name(std::move(name))
    , m_targetName(std::move(targetName))
    , m_youngModulus(youngModulus)
    , m_poissonRatio(poissonRatio)
{
}

CaeMaterial::CaeMaterial(QString name, QString targetName, double thermalConductivity)
    : m_name(std::move(name))
    , m_targetName(std::move(targetName))
    , m_thermalConductivity(thermalConductivity)
{
}

QString CaeMaterial::name() const
{
    return m_name;
}

QString CaeMaterial::targetName() const
{
    return m_targetName;
}

double CaeMaterial::youngModulus() const
{
    return m_youngModulus;
}

double CaeMaterial::poissonRatio() const
{
    return m_poissonRatio;
}

double CaeMaterial::thermalConductivity() const
{
    return m_thermalConductivity;
}

} // namespace Cae
