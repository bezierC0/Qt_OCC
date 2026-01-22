#include "DialogCreateCircle.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QGroupBox>

DialogCreateCircle::DialogCreateCircle(QWidget* parent) : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle("Create Circle");

    auto* mainLayout = new QVBoxLayout(this);
    
    // Origin Point
    auto* groupOrigin = new QGroupBox("Center Point", this);
    auto* formLayout1 = new QFormLayout(groupOrigin);

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

    formLayout1->addRow("X:", m_spinBoxX);
    formLayout1->addRow("Y:", m_spinBoxY);
    formLayout1->addRow("Z:", m_spinBoxZ);

    mainLayout->addWidget(groupOrigin);

    // Dimensions
    auto* groupDim = new QGroupBox("Dimensions", this);
    auto* formLayout2 = new QFormLayout(groupDim);

    m_spinBoxRadius = new QDoubleSpinBox(this);
    m_spinBoxRadius->setRange(0.001, 10000.0);
    m_spinBoxRadius->setSingleStep(1.0);
    m_spinBoxRadius->setValue(25.0);

    formLayout2->addRow("Radius:", m_spinBoxRadius);

    mainLayout->addWidget(groupDim);

    // Color
    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreateCircle::onBtnColorClicked);
    colorLayout->addWidget(new QLabel("Color:"));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    // Buttons
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

DialogCreateCircle::~DialogCreateCircle()
{
}

double DialogCreateCircle::x() const
{
    return m_spinBoxX->value();
}

double DialogCreateCircle::y() const
{
    return m_spinBoxY->value();
}

double DialogCreateCircle::z() const
{
    return m_spinBoxZ->value();
}

double DialogCreateCircle::radius() const
{
    return m_spinBoxRadius->value();
}

QColor DialogCreateCircle::color() const
{
    return m_color;
}

void DialogCreateCircle::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) {
        m_color = c;
        QString style = QString("background-color: %1").arg(c.name());
        m_btnColor->setStyleSheet(style);
    }
}
