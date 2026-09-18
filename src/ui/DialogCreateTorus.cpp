#include "DialogCreateTorus.h"
#include <QIcon>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QGroupBox>
#include <QMessageBox>

DialogCreateTorus::DialogCreateTorus(QWidget *parent) : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle(tr("Create Torus"));
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    
    // Position
    auto* groupPos = new QGroupBox(tr("Position (Center)"), this);
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
    auto* groupDim = new QGroupBox(tr("Dimensions"), this);
    auto* formLayoutDim = new QFormLayout(groupDim);

    m_spinBoxMajorRadius = new QDoubleSpinBox(this);
    m_spinBoxMajorRadius->setRange(0.001, 10000.0);
    m_spinBoxMajorRadius->setSingleStep(1.0);
    m_spinBoxMajorRadius->setValue(20.0);

    m_spinBoxMinorRadius = new QDoubleSpinBox(this);
    m_spinBoxMinorRadius->setRange(0.001, 10000.0);
    m_spinBoxMinorRadius->setSingleStep(1.0);
    m_spinBoxMinorRadius->setValue(5.0);

    formLayoutDim->addRow(tr("Major Radius:"), m_spinBoxMajorRadius);
    formLayoutDim->addRow(tr("Tube Radius:"), m_spinBoxMinorRadius);
    mainLayout->addWidget(groupDim);

    // Color
    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton(tr("Select Color"), this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreateTorus::onBtnColorClicked);
    colorLayout->addWidget(new QLabel(tr("Color:")));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    // Buttons
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Create"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCreateTorus::onBtnOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

DialogCreateTorus::~DialogCreateTorus()
{
}

double DialogCreateTorus::x() const
{
    return m_spinBoxX->value();
}

double DialogCreateTorus::y() const
{
    return m_spinBoxY->value();
}

double DialogCreateTorus::z() const
{
    return m_spinBoxZ->value();
}

double DialogCreateTorus::majorRadius() const
{
    return m_spinBoxMajorRadius->value();
}

double DialogCreateTorus::minorRadius() const
{
    return m_spinBoxMinorRadius->value();
}

QColor DialogCreateTorus::color() const
{
    return m_color;
}

void DialogCreateTorus::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, tr("Select Color"));
    if (c.isValid()) {
        m_color = c;
        QString style = QString("background-color: %1").arg(c.name());
        m_btnColor->setStyleSheet(style);
    }
}

void DialogCreateTorus::onBtnOkClicked()
{
    if (majorRadius() <= minorRadius()) {
        QMessageBox::warning(this, tr("Invalid Parameters"),
                             tr("Major radius must be greater than tube radius."));
        return;
    }
    emit signalCreateTorus(x(), y(), z(), majorRadius(), minorRadius(), color());
}
