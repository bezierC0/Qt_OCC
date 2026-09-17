#include "DialogCreateParabola.h"
#include "ShapePickSession.h"
#include "command/CommandCommon.h"
#include "command/ShapeCommandRegistry.h"

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
#include <QMessageBox>

DialogCreateParabola::DialogCreateParabola(QWidget* parent)
    : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle(tr("Create Parabola"));
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    m_statusLabel = new QLabel(tr("Step 1/2 : Click vertex point in 3D view"), this);
    m_statusLabel->setStyleSheet("color: #2196F3; font-style: italic;");
    mainLayout->addWidget(m_statusLabel);

    // Vertex
    auto* groupVertex = new QGroupBox(tr("Vertex"), this);
    auto* formLayoutVertex = new QFormLayout(groupVertex);
    m_spinBoxVertexX = new QDoubleSpinBox(this);
    m_spinBoxVertexX->setRange(-10000.0, 10000.0); m_spinBoxVertexX->setSingleStep(1.0); m_spinBoxVertexX->setValue(0.0);
    m_spinBoxVertexY = new QDoubleSpinBox(this);
    m_spinBoxVertexY->setRange(-10000.0, 10000.0); m_spinBoxVertexY->setSingleStep(1.0); m_spinBoxVertexY->setValue(0.0);
    m_spinBoxVertexZ = new QDoubleSpinBox(this);
    m_spinBoxVertexZ->setRange(-10000.0, 10000.0); m_spinBoxVertexZ->setSingleStep(1.0); m_spinBoxVertexZ->setValue(0.0);
    formLayoutVertex->addRow("X:", m_spinBoxVertexX);
    formLayoutVertex->addRow("Y:", m_spinBoxVertexY);
    formLayoutVertex->addRow("Z:", m_spinBoxVertexZ);
    mainLayout->addWidget(groupVertex);

    // Normal Direction
    auto* groupNormal = new QGroupBox(tr("Normal Direction"), this);
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
    auto* groupGeom = new QGroupBox(tr("Dimensions"), this);
    auto* formLayoutGeom = new QFormLayout(groupGeom);
    m_spinBoxFocalLength = new QDoubleSpinBox(this);
    m_spinBoxFocalLength->setRange(0.01, 10000.0); m_spinBoxFocalLength->setSingleStep(1.0); m_spinBoxFocalLength->setValue(15.0);
    formLayoutGeom->addRow(tr("Focal length:"), m_spinBoxFocalLength);
    m_spinBoxFirstParameter = new QDoubleSpinBox(this);
    m_spinBoxFirstParameter->setRange(-10000.0, 10000.0);
    m_spinBoxFirstParameter->setSingleStep(0.1);
    m_spinBoxFirstParameter->setValue(-30.0);
    m_spinBoxLastParameter = new QDoubleSpinBox(this);
    m_spinBoxLastParameter->setRange(-10000.0, 10000.0);
    m_spinBoxLastParameter->setSingleStep(0.1);
    m_spinBoxLastParameter->setValue(30.0);
    formLayoutGeom->addRow(tr("Start parameter u:"), m_spinBoxFirstParameter);
    formLayoutGeom->addRow(tr("End parameter u:"), m_spinBoxLastParameter);
    mainLayout->addWidget(groupGeom);

    // Color picker
    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton(tr("Select Color"), this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreateParabola::onBtnColorClicked);
    colorLayout->addWidget(new QLabel(tr("Color:")));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    // OK / Cancel
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Create"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCreateParabola::onBtnOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // Pick the vertex, then set the focal length by distance.
    m_session = new ShapePickSession(2,
        [this](const std::vector<gp_Pnt>& pts, const gp_Pnt& mouse) -> TopoDS_Shape {
            const double focal = pts[0].Distance(mouse);
            CoreApi::ShapeParams p;
            p[CoreApi::Param::X] = pts[0].X(); p[CoreApi::Param::Y] = pts[0].Y(); p[CoreApi::Param::Z] = pts[0].Z();
            p[CoreApi::Param::NX] = m_spinBoxNormalX->value();
            p[CoreApi::Param::NY] = m_spinBoxNormalY->value();
            p[CoreApi::Param::NZ] = m_spinBoxNormalZ->value();
            p[CoreApi::Param::FOCAL] = focal;
            p[CoreApi::Param::FIRST_PARAMETER] = firstParameter();
            p[CoreApi::Param::LAST_PARAMETER] = lastParameter();
            return CoreApi::ShapeCommandRegistry::instance().execute("CreateParabola", p);
        }, this);

    connect(m_session, &ShapePickSession::sessionCompleted, this, &DialogCreateParabola::onSessionCompleted);
    connect(m_session, &ShapePickSession::stateChanged, this, [this](ShapePickSession::State s) {
        if (s == ShapePickSession::State::Preview)
            m_statusLabel->setText(tr("Step 2/2 : Drag to set focal length, then click"));
        else if (s == ShapePickSession::State::Picking)
            m_statusLabel->setText(tr("Step 1/2 : Click vertex point in 3D view"));
    });
}

DialogCreateParabola::~DialogCreateParabola() {}

void DialogCreateParabola::show() { QDialog::show(); if (m_session) m_session->start(); }

void DialogCreateParabola::done(int result)
{
    if (m_session) m_session->stop();
    QDialog::done(result);
}

double DialogCreateParabola::vertexX()     const { return m_spinBoxVertexX->value(); }
double DialogCreateParabola::vertexY()     const { return m_spinBoxVertexY->value(); }
double DialogCreateParabola::vertexZ()     const { return m_spinBoxVertexZ->value(); }
double DialogCreateParabola::normalX()     const { return m_spinBoxNormalX->value(); }
double DialogCreateParabola::normalY()     const { return m_spinBoxNormalY->value(); }
double DialogCreateParabola::normalZ()     const { return m_spinBoxNormalZ->value(); }
double DialogCreateParabola::focalLength() const { return m_spinBoxFocalLength->value(); }
double DialogCreateParabola::firstParameter() const { return m_spinBoxFirstParameter->value(); }
double DialogCreateParabola::lastParameter() const { return m_spinBoxLastParameter->value(); }
QColor DialogCreateParabola::color()       const { return m_color; }

void DialogCreateParabola::closeEvent(QCloseEvent* event)
{
    if (m_session && m_session->isActive()) m_session->stop();
    QDialog::closeEvent(event);
}

void DialogCreateParabola::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, tr("Select Color"));
    if (c.isValid()) { m_color = c; m_btnColor->setStyleSheet(QString("background-color: %1").arg(c.name())); }
}

void DialogCreateParabola::onBtnOkClicked()
{
    if (firstParameter() >= lastParameter()
        || (normalX() == 0.0 && normalY() == 0.0 && normalZ() == 0.0)) {
        QMessageBox::warning(this, tr("Invalid Parameters"),
                             tr("The normal must be nonzero and the start parameter must be less than the end parameter."));
        return;
    }
    if (m_session) m_session->stop();
    emit signalCreateParabola(vertexX(), vertexY(), vertexZ(),
                             normalX(), normalY(), normalZ(),
                             focalLength(), firstParameter(), lastParameter(), color());
}

void DialogCreateParabola::onSessionCompleted(QVector<gp_Pnt> points)
{
    if (points.size() < 2) return;
    const double focal = points[0].Distance(points[1]);
    m_spinBoxVertexX->setValue(points[0].X());
    m_spinBoxVertexY->setValue(points[0].Y());
    m_spinBoxVertexZ->setValue(points[0].Z());
    if (focal > 0.001) {
        m_spinBoxFocalLength->setValue(focal);
    }
    m_statusLabel->setText(tr("Parabola defined. Press [Create] to confirm."));
}
