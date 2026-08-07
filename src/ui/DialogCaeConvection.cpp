#include "DialogCaeConvection.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>

DialogCaeConvection::DialogCaeConvection(QWidget* parent)
    : DialogCaeConvection(10.0, 20.0, parent)
{
}

DialogCaeConvection::DialogCaeConvection(
    double filmCoefficient,
    double ambientTemperature,
    QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Add Convection"));

    m_filmCoefficientSpinBox = new QDoubleSpinBox(this);
    m_filmCoefficientSpinBox->setRange(1.0e-9, 1.0e12);
    m_filmCoefficientSpinBox->setDecimals(6);
    m_filmCoefficientSpinBox->setValue(filmCoefficient);
    m_filmCoefficientSpinBox->setSuffix(tr(" W/(m^2*K)"));

    m_ambientTemperatureSpinBox = new QDoubleSpinBox(this);
    m_ambientTemperatureSpinBox->setRange(-273.15, 1.0e6);
    m_ambientTemperatureSpinBox->setDecimals(6);
    m_ambientTemperatureSpinBox->setValue(ambientTemperature);
    m_ambientTemperatureSpinBox->setSuffix(tr(" C"));

    auto* formLayout = new QFormLayout;
    formLayout->addRow(tr("Film coefficient"), m_filmCoefficientSpinBox);
    formLayout->addRow(tr("Ambient temperature"), m_ambientTemperatureSpinBox);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCaeConvection::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DialogCaeConvection::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
    layout->addWidget(buttonBox);
}

double DialogCaeConvection::filmCoefficient() const
{
    return m_filmCoefficientSpinBox->value();
}

double DialogCaeConvection::ambientTemperature() const
{
    return m_ambientTemperatureSpinBox->value();
}
