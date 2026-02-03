#include "DialogCreateBox.h"
#include <QIcon>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>

DialogCreateBox::DialogCreateBox(QWidget *parent) : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle("Create Box");
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    auto* formLayout = new QFormLayout();

    // Location
    m_spinBoxX = new QDoubleSpinBox(this);
    m_spinBoxX->setRange(-10000.0, 10000.0);
    m_spinBoxX->setSingleStep(1.0);
    m_spinBoxX->setValue(0.0);

    m_spinBoxY = new QDoubleSpinBox(this);
    m_spinBoxY->setRange(-10000.0, 10000.0);
    m_spinBoxY->setSingleStep(1.0);
    m_spinBoxY->setValue(0.0);

    m_spinBoxZ = new QDoubleSpinBox(this);
    m_spinBoxZ->setRange(-10000.0, 10000.0);
    m_spinBoxZ->setSingleStep(1.0);
    m_spinBoxZ->setValue(0.0);

    // Dimensions
    m_spinBoxDX = new QDoubleSpinBox(this);
    m_spinBoxDX->setRange(0.001, 10000.0); // positive size
    m_spinBoxDX->setSingleStep(1.0);
    m_spinBoxDX->setValue(10.0);

    m_spinBoxDY = new QDoubleSpinBox(this);
    m_spinBoxDY->setRange(0.001, 10000.0); // positive size
    m_spinBoxDY->setSingleStep(1.0);
    m_spinBoxDY->setValue(10.0);

    m_spinBoxDZ = new QDoubleSpinBox(this);
    m_spinBoxDZ->setRange(0.001, 10000.0); // positive size
    m_spinBoxDZ->setSingleStep(1.0);
    m_spinBoxDZ->setValue(10.0);

    formLayout->addRow("X:", m_spinBoxX);
    formLayout->addRow("Y:", m_spinBoxY);
    formLayout->addRow("Z:", m_spinBoxZ);
    formLayout->addRow("DX:", m_spinBoxDX);
    formLayout->addRow("DY:", m_spinBoxDY);
    formLayout->addRow("DZ:", m_spinBoxDZ);

    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreateBox::onBtnColorClicked);
    formLayout->addRow("Color:", m_btnColor);

    mainLayout->addLayout(formLayout);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Create");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCreateBox::onBtnOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

DialogCreateBox::~DialogCreateBox()
{
}

double DialogCreateBox::x() const
{
    return m_spinBoxX->value();
}

double DialogCreateBox::y() const
{
    return m_spinBoxY->value();
}

double DialogCreateBox::z() const
{
    return m_spinBoxZ->value();
}

double DialogCreateBox::dx() const
{
    return m_spinBoxDX->value();
}

double DialogCreateBox::dy() const
{
    return m_spinBoxDY->value();
}

double DialogCreateBox::dz() const
{
    return m_spinBoxDZ->value();
}

QColor DialogCreateBox::color() const
{
    return m_color;
}

void DialogCreateBox::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) {
        m_color = c;
        // Optionally update button style to show color
        QString style = QString("background-color: %1").arg(c.name());
        m_btnColor->setStyleSheet(style);
    }
}

void DialogCreateBox::onBtnOkClicked()
{
    emit signalCreateBox(x(), y(), z(), dx(), dy(), dz(), color());
}
