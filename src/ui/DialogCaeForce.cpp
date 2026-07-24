#include "DialogCaeForce.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>

DialogCaeForce::DialogCaeForce(
    const std::array<double, 3>& initialForce,
    QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Add CAE Force"));

    auto* formLayout = new QFormLayout;
    const std::array<QString, 3> labels{
        tr("Fx"), tr("Fy"), tr("Fz")};
    for (int axis = 0; axis < 3; ++axis) {
        auto* spinBox = new QDoubleSpinBox(this);
        spinBox->setRange(-1.0e12, 1.0e12);
        spinBox->setDecimals(3);
        spinBox->setValue(initialForce[axis]);
        spinBox->setSuffix(tr(" N"));
        m_componentSpinBoxes[axis] = spinBox;
        formLayout->addRow(labels[axis], spinBox);
    }

    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        const auto value = force();
        if (value[0] == 0.0 && value[1] == 0.0 && value[2] == 0.0) {
            QMessageBox::warning(
                this,
                tr("Add CAE Force"),
                tr("At least one force component must be non-zero."));
            return;
        }
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DialogCaeForce::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(formLayout);
    layout->addWidget(buttonBox);
}

std::array<double, 3> DialogCaeForce::force() const
{
    return {
        m_componentSpinBoxes[0]->value(),
        m_componentSpinBoxes[1]->value(),
        m_componentSpinBoxes[2]->value()};
}
