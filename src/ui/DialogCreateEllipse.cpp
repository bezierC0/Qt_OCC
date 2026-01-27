#include "DialogCreateEllipse.h"
#include <QIcon>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QGroupBox>

DialogCreateEllipse::DialogCreateEllipse(QWidget *parent) : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle("Create Ellipse");
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    
    // Center
    auto* groupCenter = new QGroupBox("Center", this);
    auto* formLayoutCenter = new QFormLayout(groupCenter);

    m_spinBoxCenterX = new QDoubleSpinBox(this);
    m_spinBoxCenterX->setRange(-10000.0, 10000.0);
    m_spinBoxCenterX->setSingleStep(1.0);
    m_spinBoxCenterX->setValue(0.0);

    m_spinBoxCenterY = new QDoubleSpinBox(this);
    m_spinBoxCenterY->setRange(-10000.0, 10000.0);
    m_spinBoxCenterY->setSingleStep(1.0);
    m_spinBoxCenterY->setValue(0.0);

    m_spinBoxCenterZ = new QDoubleSpinBox(this);
    m_spinBoxCenterZ->setRange(-10000.0, 10000.0);
    m_spinBoxCenterZ->setSingleStep(1.0);
    m_spinBoxCenterZ->setValue(0.0);

    formLayoutCenter->addRow("X:", m_spinBoxCenterX);
    formLayoutCenter->addRow("Y:", m_spinBoxCenterY);
    formLayoutCenter->addRow("Z:", m_spinBoxCenterZ);

    mainLayout->addWidget(groupCenter);

    // Normal Direction
    auto* groupNormal = new QGroupBox("Normal Direction", this);
    auto* formLayoutNormal = new QFormLayout(groupNormal);

    m_spinBoxNormalX = new QDoubleSpinBox(this);
    m_spinBoxNormalX->setRange(-1.0, 1.0);
    m_spinBoxNormalX->setSingleStep(0.1);
    m_spinBoxNormalX->setValue(0.0);

    m_spinBoxNormalY = new QDoubleSpinBox(this);
    m_spinBoxNormalY->setRange(-1.0, 1.0);
    m_spinBoxNormalY->setSingleStep(0.1);
    m_spinBoxNormalY->setValue(0.0);

    m_spinBoxNormalZ = new QDoubleSpinBox(this);
    m_spinBoxNormalZ->setRange(-1.0, 1.0);
    m_spinBoxNormalZ->setSingleStep(0.1);
    m_spinBoxNormalZ->setValue(1.0);

    formLayoutNormal->addRow("X:", m_spinBoxNormalX);
    formLayoutNormal->addRow("Y:", m_spinBoxNormalY);
    formLayoutNormal->addRow("Z:", m_spinBoxNormalZ);

    mainLayout->addWidget(groupNormal);

    // Geometry
    auto* groupGeom = new QGroupBox("Dimensions", this);
    auto* formLayoutGeom = new QFormLayout(groupGeom);

    m_spinBoxMajorRadius = new QDoubleSpinBox(this);
    m_spinBoxMajorRadius->setRange(0.0, 10000.0);
    m_spinBoxMajorRadius->setSingleStep(1.0);
    m_spinBoxMajorRadius->setValue(30.0);

    m_spinBoxMinorRadius = new QDoubleSpinBox(this);
    m_spinBoxMinorRadius->setRange(0.0, 10000.0);
    m_spinBoxMinorRadius->setSingleStep(1.0);
    m_spinBoxMinorRadius->setValue(15.0);

    formLayoutGeom->addRow("Major Radius:", m_spinBoxMajorRadius);
    formLayoutGeom->addRow("Minor Radius:", m_spinBoxMinorRadius);

    mainLayout->addWidget(groupGeom);

    // Color
    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreateEllipse::onBtnColorClicked);
    colorLayout->addWidget(new QLabel("Color:"));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    // Buttons
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

DialogCreateEllipse::~DialogCreateEllipse()
{
}

double DialogCreateEllipse::centerX() const
{
    return m_spinBoxCenterX->value();
}

double DialogCreateEllipse::centerY() const
{
    return m_spinBoxCenterY->value();
}

double DialogCreateEllipse::centerZ() const
{
    return m_spinBoxCenterZ->value();
}

double DialogCreateEllipse::normalX() const
{
    return m_spinBoxNormalX->value();
}

double DialogCreateEllipse::normalY() const
{
    return m_spinBoxNormalY->value();
}

double DialogCreateEllipse::normalZ() const
{
    return m_spinBoxNormalZ->value();
}

double DialogCreateEllipse::majorRadius() const
{
    return m_spinBoxMajorRadius->value();
}

double DialogCreateEllipse::minorRadius() const
{
    return m_spinBoxMinorRadius->value();
}

QColor DialogCreateEllipse::color() const
{
    return m_color;
}

void DialogCreateEllipse::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) {
        m_color = c;
        QString style = QString("background-color: %1").arg(c.name());
        m_btnColor->setStyleSheet(style);
    }
}
