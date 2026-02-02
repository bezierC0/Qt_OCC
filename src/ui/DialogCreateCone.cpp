#include "DialogCreateCone.h"
#include <QIcon>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QGroupBox>

DialogCreateCone::DialogCreateCone(QWidget *parent) : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle("Create Cone");
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    
    // Position
    auto* groupPos = new QGroupBox("Position (Base Center)", this);
    auto* formLayoutPos = new QFormLayout(groupPos);

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

    formLayoutPos->addRow("X:", m_spinBoxX);
    formLayoutPos->addRow("Y:", m_spinBoxY);
    formLayoutPos->addRow("Z:", m_spinBoxZ);

    mainLayout->addWidget(groupPos);

    // Dimensions
    auto* groupDim = new QGroupBox("Dimensions", this);
    auto* formLayoutDim = new QFormLayout(groupDim);

    m_spinBoxRadius1 = new QDoubleSpinBox(this);
    m_spinBoxRadius1->setRange(0.0, 10000.0); // R1 can be 0 or more
    m_spinBoxRadius1->setSingleStep(1.0);
    m_spinBoxRadius1->setValue(5.0);

    m_spinBoxRadius2 = new QDoubleSpinBox(this);
    m_spinBoxRadius2->setRange(0.0, 10000.0);
    m_spinBoxRadius2->setSingleStep(1.0);
    m_spinBoxRadius2->setValue(0.0);

    m_spinBoxHeight = new QDoubleSpinBox(this);
    m_spinBoxHeight->setRange(0.001, 10000.0);
    m_spinBoxHeight->setSingleStep(1.0);
    m_spinBoxHeight->setValue(10.0);

    formLayoutDim->addRow("Radius 1:", m_spinBoxRadius1);
    formLayoutDim->addRow("Radius 2:", m_spinBoxRadius2);
    formLayoutDim->addRow("Height:", m_spinBoxHeight);
    mainLayout->addWidget(groupDim);

    // Color
    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreateCone::onBtnColorClicked);
    colorLayout->addWidget(new QLabel("Color:"));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    // Buttons
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

DialogCreateCone::~DialogCreateCone()
{
}

double DialogCreateCone::x() const
{
    return m_spinBoxX->value();
}

double DialogCreateCone::y() const
{
    return m_spinBoxY->value();
}

double DialogCreateCone::z() const
{
    return m_spinBoxZ->value();
}

double DialogCreateCone::radius1() const
{
    return m_spinBoxRadius1->value();
}

double DialogCreateCone::radius2() const
{
    return m_spinBoxRadius2->value();
}

double DialogCreateCone::height() const
{
    return m_spinBoxHeight->value();
}

QColor DialogCreateCone::color() const
{
    return m_color;
}

void DialogCreateCone::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) {
        m_color = c;
        QString style = QString("background-color: %1").arg(c.name());
        m_btnColor->setStyleSheet(style);
    }
}
