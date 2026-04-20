#include "DialogCreateCircle.h"
#include "ShapePickSession.h"
#include "core/ShapeCommandRegistry.h"

#include <QIcon>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QGroupBox>

DialogCreateCircle::DialogCreateCircle(QWidget* parent)
    : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle("Create Circle");
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    m_statusLabel = new QLabel("Step 1/2 : Click centre point in 3D view", this);
    m_statusLabel->setStyleSheet("color: #2196F3; font-style: italic;");
    mainLayout->addWidget(m_statusLabel);

    auto* groupOrigin = new QGroupBox("Center Point", this);
    auto* formLayout1 = new QFormLayout(groupOrigin);

    m_spinBoxX = new QDoubleSpinBox(this);
    m_spinBoxX->setRange(-10000.0, 10000.0); m_spinBoxX->setSingleStep(1.0); m_spinBoxX->setValue(0.0);
    m_spinBoxY = new QDoubleSpinBox(this);
    m_spinBoxY->setRange(-10000.0, 10000.0); m_spinBoxY->setSingleStep(1.0); m_spinBoxY->setValue(0.0);
    m_spinBoxZ = new QDoubleSpinBox(this);
    m_spinBoxZ->setRange(-10000.0, 10000.0); m_spinBoxZ->setSingleStep(1.0); m_spinBoxZ->setValue(0.0);

    formLayout1->addRow("X:", m_spinBoxX);
    formLayout1->addRow("Y:", m_spinBoxY);
    formLayout1->addRow("Z:", m_spinBoxZ);
    mainLayout->addWidget(groupOrigin);

    auto* groupDim = new QGroupBox("Dimensions", this);
    auto* formLayout2 = new QFormLayout(groupDim);
    m_spinBoxRadius = new QDoubleSpinBox(this);
    m_spinBoxRadius->setRange(0.001, 10000.0); m_spinBoxRadius->setSingleStep(1.0); m_spinBoxRadius->setValue(25.0);
    formLayout2->addRow("Radius:", m_spinBoxRadius);
    mainLayout->addWidget(groupDim);

    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreateCircle::onBtnColorClicked);
    colorLayout->addWidget(new QLabel("Color:"));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Create");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCreateCircle::onBtnOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // ShapePickSession: 2-point Circle. P1 = centre, mouse defines radius.
    m_session = new ShapePickSession(2,
        [](const std::vector<gp_Pnt>& pts, const gp_Pnt& mouse) -> TopoDS_Shape {
            CoreApi::ShapeParams p;
            p["x"] = pts[0].X(); p["y"] = pts[0].Y(); p["z"] = pts[0].Z();
            p["radius"] = pts[0].Distance(mouse);
            return CoreApi::ShapeCommandRegistry::instance().execute("CreateCircle", p);
        }, this);

    connect(m_session, &ShapePickSession::sessionCompleted, this, &DialogCreateCircle::onSessionCompleted);
    connect(m_session, &ShapePickSession::stateChanged, this, [this](ShapePickSession::State s) {
        if (s == ShapePickSession::State::Preview)
            m_statusLabel->setText("Step 2/2 : Drag to set radius, then click");
        else if (s == ShapePickSession::State::Idle)
            m_statusLabel->setText("Step 1/2 : Click centre point in 3D view");
    });
}

DialogCreateCircle::~DialogCreateCircle() {}

void DialogCreateCircle::show() { QDialog::show(); if (m_session) m_session->start(); }

double DialogCreateCircle::x()      const { return m_spinBoxX->value(); }
double DialogCreateCircle::y()      const { return m_spinBoxY->value(); }
double DialogCreateCircle::z()      const { return m_spinBoxZ->value(); }
double DialogCreateCircle::radius() const { return m_spinBoxRadius->value(); }
QColor DialogCreateCircle::color()  const { return m_color; }

void DialogCreateCircle::closeEvent(QCloseEvent* event)
{
    if (m_session && m_session->isActive()) m_session->stop();
    QDialog::closeEvent(event);
}

void DialogCreateCircle::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) { m_color = c; m_btnColor->setStyleSheet(QString("background-color: %1").arg(c.name())); }
}

void DialogCreateCircle::onBtnOkClicked()
{
    emit signalCreateCircle(x(), y(), z(), radius(), color());
}

void DialogCreateCircle::onSessionCompleted(QVector<gp_Pnt> points)
{
    if (points.size() < 2) return;
    const double r = points[0].Distance(points[1]);
    m_spinBoxX->setValue(points[0].X());
    m_spinBoxY->setValue(points[0].Y());
    m_spinBoxZ->setValue(points[0].Z());
    if (r > 0.001) m_spinBoxRadius->setValue(r);
    m_statusLabel->setText("Circle defined. Press [Create] to confirm.");
}
