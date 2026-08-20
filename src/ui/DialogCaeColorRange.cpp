#include "DialogCaeColorRange.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSpinBox>
#include <QVBoxLayout>

DialogCaeColorRange::DialogCaeColorRange(
    bool automatic,
    double minimum,
    double maximum,
    double dataMinimum,
    double dataMaximum,
    int bandCount,
    const QString& unit,
    QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("CAE Color Map"));

    m_automaticCheckBox = new QCheckBox(tr("Use result minimum and maximum"), this);
    m_automaticCheckBox->setChecked(automatic);

    m_minimumSpinBox = new QDoubleSpinBox(this);
    m_minimumSpinBox->setRange(-1.0e15, 1.0e15);
    m_minimumSpinBox->setDecimals(12);
    m_minimumSpinBox->setValue(minimum);
    m_minimumSpinBox->setSuffix(unit.isEmpty() ? QString() : QStringLiteral(" %1").arg(unit));

    m_maximumSpinBox = new QDoubleSpinBox(this);
    m_maximumSpinBox->setRange(-1.0e15, 1.0e15);
    m_maximumSpinBox->setDecimals(12);
    m_maximumSpinBox->setValue(maximum);
    m_maximumSpinBox->setSuffix(unit.isEmpty() ? QString() : QStringLiteral(" %1").arg(unit));

    m_bandCountSpinBox = new QSpinBox(this);
    m_bandCountSpinBox->setRange(2, 32);
    m_bandCountSpinBox->setValue(bandCount);

    auto* dataRangeLabel = new QLabel(
        tr("Result range: %1 to %2 %3")
            .arg(QString::number(dataMinimum, 'g', 12))
            .arg(QString::number(dataMaximum, 'g', 12))
            .arg(unit),
        this);

    auto* formLayout = new QFormLayout;
    formLayout->addRow(tr("Display minimum"), m_minimumSpinBox);
    formLayout->addRow(tr("Display maximum"), m_maximumSpinBox);
    formLayout->addRow(tr("Color bands"), m_bandCountSpinBox);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCaeColorRange::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DialogCaeColorRange::reject);
    connect(m_automaticCheckBox, &QCheckBox::toggled, this, &DialogCaeColorRange::updateInputState);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_automaticCheckBox);
    layout->addWidget(dataRangeLabel);
    layout->addLayout(formLayout);
    layout->addWidget(buttonBox);
    updateInputState(automatic);
}

bool DialogCaeColorRange::isAutomatic() const
{
    return m_automaticCheckBox->isChecked();
}

double DialogCaeColorRange::minimum() const
{
    return m_minimumSpinBox->value();
}

double DialogCaeColorRange::maximum() const
{
    return m_maximumSpinBox->value();
}

int DialogCaeColorRange::bandCount() const
{
    return m_bandCountSpinBox->value();
}

void DialogCaeColorRange::updateInputState(bool automatic)
{
    m_minimumSpinBox->setEnabled(!automatic);
    m_maximumSpinBox->setEnabled(!automatic);
}

void DialogCaeColorRange::accept()
{
    if (!isAutomatic() && minimum() >= maximum()) {
        QMessageBox::warning(
            this,
            tr("Invalid Color Range"),
            tr("Display maximum must be greater than display minimum."));
        return;
    }
    QDialog::accept();
}
