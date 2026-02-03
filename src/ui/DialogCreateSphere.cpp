#include "DialogCreateSphere.h"
#include <QIcon>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QGroupBox>

DialogCreateSphere::DialogCreateSphere(QWidget *parent) : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle("Create Sphere");
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    
    // Position
    auto* groupPos = new QGroupBox("Position (Center)", this);
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

    m_spinBoxRadius = new QDoubleSpinBox(this);
    m_spinBoxRadius->setRange(0.001, 10000.0);
    m_spinBoxRadius->setSingleStep(1.0);
    m_spinBoxRadius->setValue(5.0);

    formLayoutDim->addRow("Radius:", m_spinBoxRadius);
    mainLayout->addWidget(groupDim);

    // Color
    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreateSphere::onBtnColorClicked);
    colorLayout->addWidget(new QLabel("Color:"));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    // Buttons
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Create");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCreateSphere::onBtnOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

DialogCreateSphere::~DialogCreateSphere()
{
}

double DialogCreateSphere::x() const
{
    return m_spinBoxX->value();
}

double DialogCreateSphere::y() const
{
    return m_spinBoxY->value();
}

double DialogCreateSphere::z() const
{
    return m_spinBoxZ->value();
}

double DialogCreateSphere::radius() const
{
    return m_spinBoxRadius->value();
}

QColor DialogCreateSphere::color() const
{
    return m_color;
}

void DialogCreateSphere::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) {
        m_color = c;
        QString style = QString("background-color: %1").arg(c.name());
        m_btnColor->setStyleSheet(style);
    }
}

void DialogCreateSphere::onBtnOkClicked()
{
    emit signalCreateSphere(x(), y(), z(), radius(), color());
}
