#include "DialogCreateLine.h"
#include <QIcon>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QGroupBox>

DialogCreateLine::DialogCreateLine(QWidget *parent) : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle("Create Line");
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    
    // Point 1
    auto* group1 = new QGroupBox("Start Point", this);
    auto* formLayout1 = new QFormLayout(group1);

    m_spinBoxX1 = new QDoubleSpinBox(this);
    m_spinBoxX1->setRange(-10000.0, 10000.0);
    m_spinBoxX1->setSingleStep(1.0);
    m_spinBoxX1->setValue(0.0);

    m_spinBoxY1 = new QDoubleSpinBox(this);
    m_spinBoxY1->setRange(-10000.0, 10000.0);
    m_spinBoxY1->setSingleStep(1.0);
    m_spinBoxY1->setValue(0.0);

    m_spinBoxZ1 = new QDoubleSpinBox(this);
    m_spinBoxZ1->setRange(-10000.0, 10000.0);
    m_spinBoxZ1->setSingleStep(1.0);
    m_spinBoxZ1->setValue(0.0);

    formLayout1->addRow("X:", m_spinBoxX1);
    formLayout1->addRow("Y:", m_spinBoxY1);
    formLayout1->addRow("Z:", m_spinBoxZ1);

    mainLayout->addWidget(group1);

    // Point 2
    auto* group2 = new QGroupBox("End Point", this);
    auto* formLayout2 = new QFormLayout(group2);

    m_spinBoxX2 = new QDoubleSpinBox(this);
    m_spinBoxX2->setRange(-10000.0, 10000.0);
    m_spinBoxX2->setSingleStep(1.0);
    m_spinBoxX2->setValue(50.0);

    m_spinBoxY2 = new QDoubleSpinBox(this);
    m_spinBoxY2->setRange(-10000.0, 10000.0);
    m_spinBoxY2->setSingleStep(1.0);
    m_spinBoxY2->setValue(50.0);

    m_spinBoxZ2 = new QDoubleSpinBox(this);
    m_spinBoxZ2->setRange(-10000.0, 10000.0);
    m_spinBoxZ2->setSingleStep(1.0);
    m_spinBoxZ2->setValue(50.0);

    formLayout2->addRow("X:", m_spinBoxX2);
    formLayout2->addRow("Y:", m_spinBoxY2);
    formLayout2->addRow("Z:", m_spinBoxZ2);

    mainLayout->addWidget(group2);

    // Color
    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreateLine::onBtnColorClicked);
    colorLayout->addWidget(new QLabel("Color:"));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    // Buttons
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

DialogCreateLine::~DialogCreateLine()
{
}

double DialogCreateLine::x1() const
{
    return m_spinBoxX1->value();
}

double DialogCreateLine::y1() const
{
    return m_spinBoxY1->value();
}

double DialogCreateLine::z1() const
{
    return m_spinBoxZ1->value();
}

double DialogCreateLine::x2() const
{
    return m_spinBoxX2->value();
}

double DialogCreateLine::y2() const
{
    return m_spinBoxY2->value();
}

double DialogCreateLine::z2() const
{
    return m_spinBoxZ2->value();
}

QColor DialogCreateLine::color() const
{
    return m_color;
}

void DialogCreateLine::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) {
        m_color = c;
        QString style = QString("background-color: %1").arg(c.name());
        m_btnColor->setStyleSheet(style);
    }
}
