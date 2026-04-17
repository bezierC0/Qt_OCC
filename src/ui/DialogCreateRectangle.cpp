#include "DialogCreateRectangle.h"
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
#include <BRepBuilderAPI_MakeWire.hxx>
#include <Precision.hxx>
#include <gp_Pnt.hxx>

// -----------------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------------

DialogCreateRectangle::DialogCreateRectangle(QWidget* parent)
    : QDialog(parent)
    , m_color(Qt::white)
{
    setWindowTitle("Create Rectangle");
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint
                   | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    // Status hint label
    m_statusLabel = new QLabel("Step 1/2 : Click origin corner in 3D view", this);
    m_statusLabel->setStyleSheet("color: #2196F3; font-style: italic;");
    mainLayout->addWidget(m_statusLabel);

    // Origin point
    auto* groupOrigin  = new QGroupBox("Origin Point", this);
    auto* formLayout1  = new QFormLayout(groupOrigin);

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
    auto* groupDim    = new QGroupBox("Dimensions (XY Plane)", this);
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

    // Color picker
    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked,
            this, &DialogCreateRectangle::onBtnColorClicked);
    colorLayout->addWidget(new QLabel("Color:"));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    // OK / Cancel buttons
    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Create");
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &DialogCreateRectangle::onBtnOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // ShapePickSession: 2-point Rectangle builder.
    // P1 = origin corner; mousePt = opposite corner.
    // The rectangle is built in the XY plane at Z = P1.Z().
    m_session = new ShapePickSession(
        2,
        [](const std::vector<gp_Pnt>& confirmedPts, const gp_Pnt& mousePt) -> TopoDS_Shape
        {
            const gp_Pnt& p1 = confirmedPts[0];
            const double  z  = p1.Z();

            const double x1 = p1.X(),   y1 = p1.Y();
            const double x2 = mousePt.X(), y2 = mousePt.Y();

            // Degenerate rectangle guard.
            if (std::abs(x2 - x1) < Precision::Confusion() ||
                std::abs(y2 - y1) < Precision::Confusion()) {
                return TopoDS_Shape{};
            }

            // Four corners.
            gp_Pnt a(x1, y1, z), b(x2, y1, z), c(x2, y2, z), d(x1, y2, z);

            BRepBuilderAPI_MakeEdge e1(a, b), e2(b, c), e3(c, d), e4(d, a);
            if (!e1.IsDone() || !e2.IsDone() || !e3.IsDone() || !e4.IsDone()) {
                return TopoDS_Shape{};
            }

            BRepBuilderAPI_MakeWire wireMaker;
            wireMaker.Add(e1.Edge());
            wireMaker.Add(e2.Edge());
            wireMaker.Add(e3.Edge());
            wireMaker.Add(e4.Edge());

            return wireMaker.IsDone() ? wireMaker.Shape() : TopoDS_Shape{};
        },
        this);

    connect(m_session, &ShapePickSession::sessionCompleted,
            this,      &DialogCreateRectangle::onSessionCompleted);

    connect(m_session, &ShapePickSession::stateChanged,
            this, [this](ShapePickSession::State s) {
                if (s == ShapePickSession::State::Preview) {
                    m_statusLabel->setText("Step 2/2 : Click opposite corner in 3D view");
                } else if (s == ShapePickSession::State::Idle) {
                    m_statusLabel->setText("Step 1/2 : Click origin corner in 3D view");
                }
            });
}

DialogCreateRectangle::~DialogCreateRectangle()
{
}

// -----------------------------------------------------------------------------
// Public interface
// -----------------------------------------------------------------------------

void DialogCreateRectangle::show()
{
    QDialog::show();
    if (m_session) {
        m_session->start();
    }
}

double DialogCreateRectangle::x()      const { return m_spinBoxX->value();      }
double DialogCreateRectangle::y()      const { return m_spinBoxY->value();      }
double DialogCreateRectangle::z()      const { return m_spinBoxZ->value();      }
double DialogCreateRectangle::width()  const { return m_spinBoxWidth->value();  }
double DialogCreateRectangle::height() const { return m_spinBoxHeight->value(); }
QColor DialogCreateRectangle::color()  const { return m_color;                  }

// -----------------------------------------------------------------------------
// Protected / Private slots
// -----------------------------------------------------------------------------

void DialogCreateRectangle::closeEvent(QCloseEvent* event)
{
    if (m_session && m_session->isActive()) {
        m_session->stop();
    }
    QDialog::closeEvent(event);
}

void DialogCreateRectangle::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) {
        m_color = c;
        m_btnColor->setStyleSheet(
            QString("background-color: %1").arg(c.name()));
    }
}

void DialogCreateRectangle::onBtnOkClicked()
{
    emit signalCreateRectangle(x(), y(), z(), width(), height(), color());
}

void DialogCreateRectangle::onSessionCompleted(QVector<gp_Pnt> points)
{
    // points[0] = origin corner, points[1] = opposite corner.
    if (points.size() < 2) {
        return;
    }

    const gp_Pnt& p1 = points[0];
    const gp_Pnt& p2 = points[1];

    // Populate Origin SpinBoxes with the first picked point.
    m_spinBoxX->setValue(p1.X());
    m_spinBoxY->setValue(p1.Y());
    m_spinBoxZ->setValue(p1.Z());

    // Derive Width and Height from the diagonal.
    m_spinBoxWidth->setValue(std::abs(p2.X() - p1.X()));
    m_spinBoxHeight->setValue(std::abs(p2.Y() - p1.Y()));

    m_statusLabel->setText("Rectangle defined. Press [Create] to confirm.");
}
