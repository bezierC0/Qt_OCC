#include "DialogCreatePoint.h"
#include "ViewerPickHelper.h"
#include "command/ShapeCommandRegistry.h"
#include <QIcon>
#include <QCloseEvent>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>

DialogCreatePoint::DialogCreatePoint(QWidget *parent) : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle("Create Point");
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    auto* formLayout = new QFormLayout();

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

    formLayout->addRow("X:", m_spinBoxX);
    formLayout->addRow("Y:", m_spinBoxY);
    formLayout->addRow("Z:", m_spinBoxZ);

    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreatePoint::onBtnColorClicked);
    formLayout->addRow("Color:", m_btnColor);

    mainLayout->addLayout(formLayout);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Create");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCreatePoint::onBtnOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    m_pickHelper = new ViewerPickHelper(this);
    connect(m_pickHelper, &ViewerPickHelper::pointPicked,
            this, &DialogCreatePoint::onPointPicked);
}

DialogCreatePoint::~DialogCreatePoint()
{
    if (m_pickHelper && m_pickHelper->isActive()) {
        m_pickHelper->stop();
    }
}

void DialogCreatePoint::show()
{
    QDialog::show();

    if (m_pickHelper) {
        m_pickHelper->start();
    }
}

void DialogCreatePoint::closeEvent(QCloseEvent* event)
{
    if (m_pickHelper && m_pickHelper->isActive()) {
        m_pickHelper->stop();
    }
    QDialog::closeEvent(event);
}

void DialogCreatePoint::onPointPicked(double x, double y, double z)
{
    m_spinBoxX->setValue(x);
    m_spinBoxY->setValue(y);
    m_spinBoxZ->setValue(z);

    CoreApi::ShapeParams p;
    p["x"] = x; p["y"] = y; p["z"] = z;
    const auto shape = CoreApi::ShapeCommandRegistry::instance().execute("CreatePoint", p);
    if (!shape.IsNull() && m_pickHelper) {
        m_pickHelper->setPreviewShape(shape, m_color.redF(), m_color.greenF(), m_color.blueF());
    }
}

double DialogCreatePoint::x() const
{
    return m_spinBoxX->value();
}

double DialogCreatePoint::y() const
{
    return m_spinBoxY->value();
}

double DialogCreatePoint::z() const
{
    return m_spinBoxZ->value();
}

QColor DialogCreatePoint::color() const
{
    return m_color;
}

void DialogCreatePoint::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) {
        m_color = c;
        // Optionally update button style to show color
        QString style = QString("background-color: %1").arg(c.name());
        m_btnColor->setStyleSheet(style);
    }
}

void DialogCreatePoint::onBtnOkClicked()
{
    emit signalCreatePoint(x(), y(), z(), color());
}
