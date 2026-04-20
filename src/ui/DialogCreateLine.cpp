#include "DialogCreateLine.h"
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

DialogCreateLine::DialogCreateLine(QWidget* parent)
    : QDialog(parent)
    , m_color(Qt::white)
{
    setWindowTitle("Create Line");
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint
                   | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    // Status hint label
    m_statusLabel = new QLabel("Step 1/2 : Click start point in 3D view", this);
    m_statusLabel->setStyleSheet("color: #2196F3; font-style: italic;");
    mainLayout->addWidget(m_statusLabel);

    // Start point coordinates
    auto* group1      = new QGroupBox("Start Point", this);
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

    // End point coordinates
    auto* group2      = new QGroupBox("End Point", this);
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

    // Color picker
    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked,
            this, &DialogCreateLine::onBtnColorClicked);
    colorLayout->addWidget(new QLabel("Color:"));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    // OK / Cancel buttons
    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Create");
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &DialogCreateLine::onBtnOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // ShapePickSession: 2-point Line builder
    m_session = new ShapePickSession(
        2,
        [](const std::vector<gp_Pnt>& confirmedPts, const gp_Pnt& mousePt) -> TopoDS_Shape
        {
            CoreApi::ShapeParams p;
            p["x1"] = confirmedPts[0].X(); p["y1"] = confirmedPts[0].Y(); p["z1"] = confirmedPts[0].Z();
            p["x2"] = mousePt.X();         p["y2"] = mousePt.Y();         p["z2"] = mousePt.Z();
            return CoreApi::ShapeCommandRegistry::instance().execute("CreateLine", p);
        },
        this);

    connect(m_session, &ShapePickSession::sessionCompleted,
            this,      &DialogCreateLine::onSessionCompleted);

    // Update the status label whenever the state machine transitions.
    connect(m_session, &ShapePickSession::stateChanged,
            this, [this](ShapePickSession::State s) {
                if (s == ShapePickSession::State::Preview) {
                    m_statusLabel->setText("Step 2/2 : Click end point in 3D view");
                } else if (s == ShapePickSession::State::Idle) {
                    m_statusLabel->setText("Step 1/2 : Click start point in 3D view");
                }
            });
}

DialogCreateLine::~DialogCreateLine()
{
    // m_session is managed by the QObject parent-child mechanism.
}

// -----------------------------------------------------------------------------
// Public interface
// -----------------------------------------------------------------------------

void DialogCreateLine::show()
{
    QDialog::show();
    if (m_session) {
        m_session->start();
    }
}

double DialogCreateLine::x1() const { return m_spinBoxX1->value(); }
double DialogCreateLine::y1() const { return m_spinBoxY1->value(); }
double DialogCreateLine::z1() const { return m_spinBoxZ1->value(); }
double DialogCreateLine::x2() const { return m_spinBoxX2->value(); }
double DialogCreateLine::y2() const { return m_spinBoxY2->value(); }
double DialogCreateLine::z2() const { return m_spinBoxZ2->value(); }

QColor DialogCreateLine::color() const { return m_color; }

// -----------------------------------------------------------------------------
// Protected / Private slots
// -----------------------------------------------------------------------------

void DialogCreateLine::closeEvent(QCloseEvent* event)
{
    if (m_session && m_session->isActive()) {
        m_session->stop();
    }
    QDialog::closeEvent(event);
}

void DialogCreateLine::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) {
        m_color = c;
        m_btnColor->setStyleSheet(
            QString("background-color: %1").arg(c.name()));
    }
}

void DialogCreateLine::onBtnOkClicked()
{
    emit signalCreateLine(x1(), y1(), z1(), x2(), y2(), z2(), color());
}

void DialogCreateLine::onSessionCompleted(QVector<gp_Pnt> points)
{
    // points[0] = start point, points[1] = end point.
    if (points.size() < 2) {
        return;
    }

    m_spinBoxX1->setValue(points[0].X());
    m_spinBoxY1->setValue(points[0].Y());
    m_spinBoxZ1->setValue(points[0].Z());

    m_spinBoxX2->setValue(points[1].X());
    m_spinBoxY2->setValue(points[1].Y());
    m_spinBoxZ2->setValue(points[1].Z());

    m_statusLabel->setText("Line defined. Press [Create] to confirm.");
}
