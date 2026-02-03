#include "DialogCreateRectangle.h"
#include <QIcon>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QGroupBox>

DialogCreateRectangle::DialogCreateRectangle(QWidget* parent) : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle("Create Rectangle");
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    
    // Origin Point
    auto* groupOrigin = new QGroupBox("Origin Point", this);
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
    auto* groupDim = new QGroupBox("Dimensions (XY Plane)", this);
    auto* formLayout2 = new QFormLayout(groupDim);

    m_spinBoxWidth = new QDoubleSpinBox(this);
    m_spinBoxWidth->setRange(0.001, 10000.0);
    m_spinBoxWidth->setSingleStep(1.0);
    m_spinBoxWidth->setValue(40.0);

    m_spinBoxHeight = new QDoubleSpinBox(this);
    m_spinBoxHeight->setRange(0.001, 10000.0);
    m_spinBoxHeight->setSingleStep(1.0);
    m_spinBoxHeight->setValue(30.0);

    formLayout2->addRow("Width:", m_spinBoxWidth);
    formLayout2->addRow("Height:", m_spinBoxHeight);

    mainLayout->addWidget(groupDim);

    // Color
    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreateRectangle::onBtnColorClicked);
    colorLayout->addWidget(new QLabel("Color:"));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    // Buttons
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

DialogCreateRectangle::~DialogCreateRectangle()
{
}

double DialogCreateRectangle::x() const
{
    return m_spinBoxX->value();
}

double DialogCreateRectangle::y() const
{
    return m_spinBoxY->value();
}

double DialogCreateRectangle::z() const
{
    return m_spinBoxZ->value();
}

double DialogCreateRectangle::width() const
{
    return m_spinBoxWidth->value();
}

double DialogCreateRectangle::height() const
{
    return m_spinBoxHeight->value();
}

QColor DialogCreateRectangle::color() const
{
    return m_color;
}

void DialogCreateRectangle::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) {
        m_color = c;
        QString style = QString("background-color: %1").arg(c.name());
        m_btnColor->setStyleSheet(style);
    }
}
