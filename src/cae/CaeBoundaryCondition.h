#pragma once

#include <QString>
#include <array>

namespace Cae {

enum class BoundaryConditionType {
    FixedSupport,
    Force,
    Pressure
};

class CaeBoundaryCondition {
public:
    CaeBoundaryCondition(QString name, QString targetName, BoundaryConditionType type);
    CaeBoundaryCondition(QString name, QString targetName, BoundaryConditionType type, double value, QString unit);
    CaeBoundaryCondition(
        QString name,
        QString targetName,
        BoundaryConditionType type,
        std::array<double, 3> components,
        QString unit);

    QString name() const;
    QString targetName() const;
    BoundaryConditionType type() const;
    double value() const;
    const std::array<double, 3>& components() const;
    QString unit() const;
    QString summary() const;

private:
    QString m_name;
    QString m_targetName;
    BoundaryConditionType m_type{BoundaryConditionType::FixedSupport};
    double m_value{0.0};
    std::array<double, 3> m_components{};
    QString m_unit;
};

QString toDisplayString(BoundaryConditionType type);

} // namespace Cae
