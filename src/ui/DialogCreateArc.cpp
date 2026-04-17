#include "DialogCreateArc.h"
#include "ShapePickSession.h"

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

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Precision.hxx>
#include <gp_Pnt.hxx>

DialogCreateArc::DialogCreateArc(QWidget* parent)
    : QDialog(parent)
    , m_color(Qt::white)
{
    setWindowTitle("Create Arc");
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint
                   | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    // Status hint label
    m_statusLabel = new QLabel("Step 1/3 : Click Point 1 in 3D view", this);
    m_statusLabel->setStyleSheet("color: #2196F3; font-style: italic;");
    mainLayout->addWidget(m_statusLabel);

    // Point 1
    auto* group1      = new QGroupBox("Point 1", this);
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
    auto* group2 = new QGroupBox("Point 2", this);
    auto* formLayout2 = new QFormLayout(group2);

    m_spinBoxX2 = new QDoubleSpinBox(this);
    m_spinBoxX2->setRange(-10000.0, 10000.0);
    m_spinBoxX2->setSingleStep(1.0);
    m_spinBoxX2->setValue(30.0);

    m_spinBoxY2 = new QDoubleSpinBox(this);
    m_spinBoxY2->setRange(-10000.0, 10000.0);
    m_spinBoxY2->setSingleStep(1.0);
    m_spinBoxY2->setValue(0.0);

    m_spinBoxZ2 = new QDoubleSpinBox(this);
    m_spinBoxZ2->setRange(-10000.0, 10000.0);
    m_spinBoxZ2->setSingleStep(1.0);
    m_spinBoxZ2->setValue(0.0);

    formLayout2->addRow("X:", m_spinBoxX2);
    formLayout2->addRow("Y:", m_spinBoxY2);
    formLayout2->addRow("Z:", m_spinBoxZ2);
    mainLayout->addWidget(group2);

    // Point 3
    auto* group3 = new QGroupBox("Point 3", this);
    auto* formLayout3 = new QFormLayout(group3);

    m_spinBoxX3 = new QDoubleSpinBox(this);
    m_spinBoxX3->setRange(-10000.0, 10000.0);
    m_spinBoxX3->setSingleStep(1.0);
    m_spinBoxX3->setValue(0.0);

    m_spinBoxY3 = new QDoubleSpinBox(this);
    m_spinBoxY3->setRange(-10000.0, 10000.0);
    m_spinBoxY3->setSingleStep(1.0);
    m_spinBoxY3->setValue(30.0);

    m_spinBoxZ3 = new QDoubleSpinBox(this);
    m_spinBoxZ3->setRange(-10000.0, 10000.0);
    m_spinBoxZ3->setSingleStep(1.0);
    m_spinBoxZ3->setValue(0.0);

    formLayout3->addRow("X:", m_spinBoxX3);
    formLayout3->addRow("Y:", m_spinBoxY3);
    formLayout3->addRow("Z:", m_spinBoxZ3);
    mainLayout->addWidget(group3);

    // Color picker
    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked,
            this, &DialogCreateArc::onBtnColorClicked);
    colorLayout->addWidget(new QLabel("Color:"));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    // OK / Cancel buttons
    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Create");
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &DialogCreateArc::onBtnOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // ShapePickSession: 3-point Arc builder.
    //
    // Stage 1 (confirmedPts.size() == 1):
    //   Show a reference line segment from P1 to the mouse cursor.
    //
    // Stage 2 (confirmedPts.size() == 2):
    //   Attempt to build a circular arc through P1, P2, and the mouse cursor.
    //   Falls back to a line segment when the three points are collinear or coincident.
    m_session = new ShapePickSession(
        3,
        [](const std::vector<gp_Pnt>& confirmedPts, const gp_Pnt& mousePt) -> TopoDS_Shape
        {
            const gp_Pnt& p1 = confirmedPts[0];

            if (confirmedPts.size() == 1) {
                // Stage 1: draw a reference segment from P1 to the mouse.
                if (p1.IsEqual(mousePt, Precision::Confusion())) {
                    return TopoDS_Shape{};
                }
                BRepBuilderAPI_MakeEdge edgeMaker(p1, mousePt);
                return edgeMaker.IsDone() ? edgeMaker.Shape() : TopoDS_Shape{};
            }

            // Stage 2: try to build a 3-point arc.
            const gp_Pnt& p2 = confirmedPts[1];
            const gp_Pnt& p3 = mousePt;

            GC_MakeArcOfCircle arcMaker(p1, p2, p3);
            if (!arcMaker.IsDone()) {
                // Degenerate case (collinear / coincident): fall back to a line segment.
                if (p1.IsEqual(p3, Precision::Confusion())) {
                    return TopoDS_Shape{};
                }
                BRepBuilderAPI_MakeEdge fallback(p1, p3);
                return fallback.IsDone() ? fallback.Shape() : TopoDS_Shape{};
            }

            BRepBuilderAPI_MakeEdge arcEdge(arcMaker.Value());
            return arcEdge.IsDone() ? arcEdge.Shape() : TopoDS_Shape{};
        },
        this);

    connect(m_session, &ShapePickSession::sessionCompleted,
            this,      &DialogCreateArc::onSessionCompleted);

    // Update the status label on every state transition.
    // A local counter tracks how many times Preview has been entered to infer the step number.
    connect(m_session, &ShapePickSession::stateChanged,
            this, [this](ShapePickSession::State s) {
                static int previewCount = 0;
                if (s == ShapePickSession::State::Preview) {
                    ++previewCount;
                    m_statusLabel->setText(previewCount == 1
                        ? "Step 2/3 : Click Point 2 in 3D view"
                        : "Step 3/3 : Click Point 3 in 3D view");
                } else if (s == ShapePickSession::State::Idle) {
                    previewCount = 0;
                    m_statusLabel->setText("Step 1/3 : Click Point 1 in 3D view");
                }
            });
}

DialogCreateArc::~DialogCreateArc()
{
    // m_session is managed by the QObject parent-child mechanism.
}

double DialogCreateArc::x1() const { return m_spinBoxX1->value(); }
double DialogCreateArc::y1() const { return m_spinBoxY1->value(); }
double DialogCreateArc::z1() const { return m_spinBoxZ1->value(); }
double DialogCreateArc::x2() const { return m_spinBoxX2->value(); }
double DialogCreateArc::y2() const { return m_spinBoxY2->value(); }
double DialogCreateArc::z2() const { return m_spinBoxZ2->value(); }
double DialogCreateArc::x3() const { return m_spinBoxX3->value(); }
double DialogCreateArc::y3() const { return m_spinBoxY3->value(); }
double DialogCreateArc::z3() const { return m_spinBoxZ3->value(); }

QColor DialogCreateArc::color() const { return m_color; }

// -----------------------------------------------------------------------------
// Protected / Private slots
// -----------------------------------------------------------------------------

void DialogCreateArc::closeEvent(QCloseEvent* event)
{
    if (m_session && m_session->isActive()) {
        m_session->stop();
    }
    QDialog::closeEvent(event);
}

void DialogCreateArc::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) {
        m_color = c;
        m_btnColor->setStyleSheet(
            QString("background-color: %1").arg(c.name()));
    }
}

void DialogCreateArc::onBtnOkClicked()
{
    emit signalCreateArc(x1(), y1(), z1(),
                         x2(), y2(), z2(),
                         x3(), y3(), z3(),
                         color());
}

void DialogCreateArc::onSessionCompleted(QVector<gp_Pnt> points)
{
    // points[0] = P1, points[1] = P2, points[2] = P3.
    if (points.size() < 3) {
        return;
    }

    m_spinBoxX1->setValue(points[0].X());
    m_spinBoxY1->setValue(points[0].Y());
    m_spinBoxZ1->setValue(points[0].Z());

    m_spinBoxX2->setValue(points[1].X());
    m_spinBoxY2->setValue(points[1].Y());
    m_spinBoxZ2->setValue(points[1].Z());

    m_spinBoxX3->setValue(points[2].X());
    m_spinBoxY3->setValue(points[2].Y());
    m_spinBoxZ3->setValue(points[2].Z());

    m_statusLabel->setText("Arc defined. Press [Create] to confirm.");
}
