#pragma once

#include <QString>

namespace Cae {

class CaeMaterial {
public:
    CaeMaterial(QString name, QString targetName, double youngModulus, double poissonRatio);

    QString name() const;
    QString targetName() const;
    double youngModulus() const;
    double poissonRatio() const;

private:
    QString m_name;
    QString m_targetName;
    double m_youngModulus{0.0};
    double m_poissonRatio{0.0};
};

} // namespace Cae
