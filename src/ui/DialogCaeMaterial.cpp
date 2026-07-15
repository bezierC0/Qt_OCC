#include "DialogCaeMaterial.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

DialogCaeMaterial::DialogCaeMaterial(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Assign CAE Material"));

    m_nameEdit = new QLineEdit(tr("Default Steel"), this);
    m_youngModulusSpinBox = new QDoubleSpinBox(this);
    m_youngModulusSpinBox->setRange(1.0, 1.0e12);
    m_youngModulusSpinBox->setDecimals(3);
    m_youngModulusSpinBox->setValue(210000.0);
    m_youngModulusSpinBox->setSuffix(tr(" MPa"));
    m_poissonRatioSpinBox = new QDoubleSpinBox(this);
    m_poissonRatioSpinBox->setRange(0.0, 0.4999);
    m_poissonRatioSpinBox->setDecimals(4);
    m_poissonRatioSpinBox->setSingleStep(0.01);
    m_poissonRatioSpinBox->setValue(0.3);

    auto* formLayout = new QFormLayout;
    formLayout->addRow(tr("Name"), m_nameEdit);
    formLayout->addRow(tr("Young's modulus"), m_youngModulusSpinBox);
    formLayout->addRow(tr("Poisson ratio"), m_poissonRatioSpinBox);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCaeMaterial::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DialogCaeMaterial::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
    layout->addWidget(buttonBox);
}

QString DialogCaeMaterial::materialName() const
{
    return m_nameEdit->text().trimmed();
}

double DialogCaeMaterial::youngModulus() const
{
    return m_youngModulusSpinBox->value();
}

double DialogCaeMaterial::poissonRatio() const
{
    return m_poissonRatioSpinBox->value();
}
