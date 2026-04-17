#include "DialogCreateEllipse.h"
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
#include <GC_MakeEllipse.hxx>
#include <Precision.hxx>
#include <gp_Pnt.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>

DialogCreateEllipse::DialogCreateEllipse(QWidget* parent)
    : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle("Create Ellipse");
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    m_statusLabel = new QLabel("Step 1/2 : Click centre point in 3D view", this);
    m_statusLabel->setStyleSheet("color: #2196F3; font-style: italic;");
    mainLayout->addWidget(m_statusLabel);

    // Centre
    auto* groupCenter = new QGroupBox("Center", this);
    auto* formLayoutCenter = new QFormLayout(groupCenter);
    m_spinBoxCenterX = new QDoubleSpinBox(this);
    m_spinBoxCenterX->setRange(-10000.0, 10000.0); m_spinBoxCenterX->setSingleStep(1.0); m_spinBoxCenterX->setValue(0.0);
    m_spinBoxCenterY = new QDoubleSpinBox(this);
    m_spinBoxCenterY->setRange(-10000.0, 10000.0); m_spinBoxCenterY->setSingleStep(1.0); m_spinBoxCenterY->setValue(0.0);
    m_spinBoxCenterZ = new QDoubleSpinBox(this);
    m_spinBoxCenterZ->setRange(-10000.0, 10000.0); m_spinBoxCenterZ->setSingleStep(1.0); m_spinBoxCenterZ->setValue(0.0);
    formLayoutCenter->addRow("X:", m_spinBoxCenterX);
    formLayoutCenter->addRow("Y:", m_spinBoxCenterY);
    formLayoutCenter->addRow("Z:", m_spinBoxCenterZ);
    mainLayout->addWidget(groupCenter);

    // Normal Direction
    auto* groupNormal = new QGroupBox("Normal Direction", this);
    auto* formLayoutNormal = new QFormLayout(groupNormal);
    m_spinBoxNormalX = new QDoubleSpinBox(this);
    m_spinBoxNormalX->setRange(-1.0, 1.0); m_spinBoxNormalX->setSingleStep(0.1); m_spinBoxNormalX->setValue(0.0);
    m_spinBoxNormalY = new QDoubleSpinBox(this);
    m_spinBoxNormalY->setRange(-1.0, 1.0); m_spinBoxNormalY->setSingleStep(0.1); m_spinBoxNormalY->setValue(0.0);
    m_spinBoxNormalZ = new QDoubleSpinBox(this);
    m_spinBoxNormalZ->setRange(-1.0, 1.0); m_spinBoxNormalZ->setSingleStep(0.1); m_spinBoxNormalZ->setValue(1.0);
    formLayoutNormal->addRow("X:", m_spinBoxNormalX);
    formLayoutNormal->addRow("Y:", m_spinBoxNormalY);
    formLayoutNormal->addRow("Z:", m_spinBoxNormalZ);
    mainLayout->addWidget(groupNormal);

    // Geometry
    auto* groupGeom = new QGroupBox("Dimensions", this);
    auto* formLayoutGeom = new QFormLayout(groupGeom);
    m_spinBoxMajorRadius = new QDoubleSpinBox(this);
    m_spinBoxMajorRadius->setRange(0.0, 10000.0); m_spinBoxMajorRadius->setSingleStep(1.0); m_spinBoxMajorRadius->setValue(30.0);
    m_spinBoxMinorRadius = new QDoubleSpinBox(this);
    m_spinBoxMinorRadius->setRange(0.0, 10000.0); m_spinBoxMinorRadius->setSingleStep(1.0); m_spinBoxMinorRadius->setValue(15.0);
    formLayoutGeom->addRow("Major Radius:", m_spinBoxMajorRadius);
    formLayoutGeom->addRow("Minor Radius:", m_spinBoxMinorRadius);
    mainLayout->addWidget(groupGeom);

    // Color picker
    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreateEllipse::onBtnColorClicked);
    colorLayout->addWidget(new QLabel("Color:"));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    // OK / Cancel
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Create");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCreateEllipse::onBtnOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // ShapePickSession: 2-point Ellipse builder.
    // P1 = centre; mouse sets MajorRadius = dist(P1, mouse); MinorRadius = MajorRadius / 2.
    // The preview uses the Normal Direction SpinBoxes for the plane normal.
    m_session = new ShapePickSession(2,
        [this](const std::vector<gp_Pnt>& pts, const gp_Pnt& mouse) -> TopoDS_Shape {
            const double major = pts[0].Distance(mouse);
            if (major < Precision::Confusion()) return {};
            const double minor = major / 2.0;

            // Read the current normal direction from the SpinBoxes.
            const double nx = m_spinBoxNormalX->value();
            const double ny = m_spinBoxNormalY->value();
            const double nz = m_spinBoxNormalZ->value();
            gp_Dir normal;
            try { normal = gp_Dir(nx, ny, nz); }
            catch (...) { normal = gp_Dir(0, 0, 1); }

            gp_Ax2 ax(pts[0], normal);
            GC_MakeEllipse em(ax, major, minor);
            if (!em.IsDone()) return {};
            BRepBuilderAPI_MakeEdge edge(em.Value());
            return edge.IsDone() ? edge.Shape() : TopoDS_Shape{};
        }, this);

    connect(m_session, &ShapePickSession::sessionCompleted, this, &DialogCreateEllipse::onSessionCompleted);
    connect(m_session, &ShapePickSession::stateChanged, this, [this](ShapePickSession::State s) {
        if (s == ShapePickSession::State::Preview)
            m_statusLabel->setText("Step 2/2 : Drag to set major radius, then click");
        else if (s == ShapePickSession::State::Idle)
            m_statusLabel->setText("Step 1/2 : Click centre point in 3D view");
    });
}

DialogCreateEllipse::~DialogCreateEllipse() {}

void DialogCreateEllipse::show() { QDialog::show(); if (m_session) m_session->start(); }

double DialogCreateEllipse::centerX()     const { return m_spinBoxCenterX->value(); }
double DialogCreateEllipse::centerY()     const { return m_spinBoxCenterY->value(); }
double DialogCreateEllipse::centerZ()     const { return m_spinBoxCenterZ->value(); }
double DialogCreateEllipse::normalX()     const { return m_spinBoxNormalX->value(); }
double DialogCreateEllipse::normalY()     const { return m_spinBoxNormalY->value(); }
double DialogCreateEllipse::normalZ()     const { return m_spinBoxNormalZ->value(); }
double DialogCreateEllipse::majorRadius() const { return m_spinBoxMajorRadius->value(); }
double DialogCreateEllipse::minorRadius() const { return m_spinBoxMinorRadius->value(); }
QColor DialogCreateEllipse::color()       const { return m_color; }

void DialogCreateEllipse::closeEvent(QCloseEvent* event)
{
    if (m_session && m_session->isActive()) m_session->stop();
    QDialog::closeEvent(event);
}

void DialogCreateEllipse::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) { m_color = c; m_btnColor->setStyleSheet(QString("background-color: %1").arg(c.name())); }
}

void DialogCreateEllipse::onBtnOkClicked()
{
    emit signalCreateEllipse(centerX(), centerY(), centerZ(),
                             normalX(), normalY(), normalZ(),
                             majorRadius(), minorRadius(), color());
}

void DialogCreateEllipse::onSessionCompleted(QVector<gp_Pnt> points)
{
    if (points.size() < 2) return;
    const double major = points[0].Distance(points[1]);
    m_spinBoxCenterX->setValue(points[0].X());
    m_spinBoxCenterY->setValue(points[0].Y());
    m_spinBoxCenterZ->setValue(points[0].Z());
    if (major > Precision::Confusion()) {
        m_spinBoxMajorRadius->setValue(major);
        m_spinBoxMinorRadius->setValue(major / 2.0);
    }
    m_statusLabel->setText("Ellipse defined. Press [Create] to confirm.");
}
