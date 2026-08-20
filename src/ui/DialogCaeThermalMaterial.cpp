#include "DialogCaeThermalMaterial.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

DialogCaeThermalMaterial::DialogCaeThermalMaterial(QWidget* parent)
    : DialogCaeThermalMaterial(QStringLiteral("Default Thermal Material"), 45.0, parent)
{
}

DialogCaeThermalMaterial::DialogCaeThermalMaterial(
    const QString& name,
    double thermalConductivity,
    QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Assign Thermal Material"));

    m_nameEdit = new QLineEdit(name, this);
    m_thermalConductivitySpinBox = new QDoubleSpinBox(this);
    m_thermalConductivitySpinBox->setRange(1.0e-9, 1.0e9);
    m_thermalConductivitySpinBox->setDecimals(6);
    m_thermalConductivitySpinBox->setValue(thermalConductivity);
    m_thermalConductivitySpinBox->setSuffix(tr(" W/(m*K)"));

    auto* formLayout = new QFormLayout;
    formLayout->addRow(tr("Name"), m_nameEdit);
    formLayout->addRow(tr("Thermal conductivity"), m_thermalConductivitySpinBox);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCaeThermalMaterial::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DialogCaeThermalMaterial::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
    layout->addWidget(buttonBox);
}

QString DialogCaeThermalMaterial::materialName() const
{
    return m_nameEdit->text().trimmed();
}

double DialogCaeThermalMaterial::thermalConductivity() const
{
    return m_thermalConductivitySpinBox->value();
}
