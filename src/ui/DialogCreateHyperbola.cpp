#include "DialogCreateHyperbola.h"
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

DialogCreateHyperbola::DialogCreateHyperbola(QWidget* parent)
    : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle(tr("Create Hyperbola"));
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    m_statusLabel = new QLabel(tr("Step 1/2 : Click centre point in 3D view"), this);
    m_statusLabel->setStyleSheet("color: #2196F3; font-style: italic;");
    mainLayout->addWidget(m_statusLabel);

    // Centre
    auto* groupCenter = new QGroupBox(tr("Center"), this);
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
    m_spinBoxMajorRadius = new QDoubleSpinBox(this);
    m_spinBoxMajorRadius->setRange(0.01, 10000.0); m_spinBoxMajorRadius->setSingleStep(1.0); m_spinBoxMajorRadius->setValue(30.0);
    m_spinBoxMinorRadius = new QDoubleSpinBox(this);
    m_spinBoxMinorRadius->setRange(0.01, 10000.0); m_spinBoxMinorRadius->setSingleStep(1.0); m_spinBoxMinorRadius->setValue(15.0);
    formLayoutGeom->addRow(tr("Semi-axis a:"), m_spinBoxMajorRadius);
    formLayoutGeom->addRow(tr("Semi-axis b:"), m_spinBoxMinorRadius);
    m_spinBoxFirstParameter = new QDoubleSpinBox(this);
    m_spinBoxFirstParameter->setRange(-10.0, 10.0);
    m_spinBoxFirstParameter->setSingleStep(0.1);
    m_spinBoxFirstParameter->setValue(-1.0);
    m_spinBoxLastParameter = new QDoubleSpinBox(this);
    m_spinBoxLastParameter->setRange(-10.0, 10.0);
    m_spinBoxLastParameter->setSingleStep(0.1);
    m_spinBoxLastParameter->setValue(1.0);
    formLayoutGeom->addRow(tr("Start parameter u:"), m_spinBoxFirstParameter);
    formLayoutGeom->addRow(tr("End parameter u:"), m_spinBoxLastParameter);
    mainLayout->addWidget(groupGeom);

    // Color picker
    auto* colorLayout = new QHBoxLayout();
    m_btnColor = new QPushButton(tr("Select Color"), this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreateHyperbola::onBtnColorClicked);
    colorLayout->addWidget(new QLabel(tr("Color:")));
    colorLayout->addWidget(m_btnColor);
    mainLayout->addLayout(colorLayout);

    // OK / Cancel
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Create"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCreateHyperbola::onBtnOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // Pick the center, then set a by distance and b to a / 2.
    m_session = new ShapePickSession(2,
        [this](const std::vector<gp_Pnt>& pts, const gp_Pnt& mouse) -> TopoDS_Shape {
            const double major = pts[0].Distance(mouse);
            CoreApi::ShapeParams p;
            p[CoreApi::Param::X] = pts[0].X(); p[CoreApi::Param::Y] = pts[0].Y(); p[CoreApi::Param::Z] = pts[0].Z();
            p[CoreApi::Param::NX] = m_spinBoxNormalX->value();
            p[CoreApi::Param::NY] = m_spinBoxNormalY->value();
            p[CoreApi::Param::NZ] = m_spinBoxNormalZ->value();
            p[CoreApi::Param::MAJOR] = major;
            p[CoreApi::Param::MINOR] = major / 2.0;
            p[CoreApi::Param::FIRST_PARAMETER] = firstParameter();
            p[CoreApi::Param::LAST_PARAMETER] = lastParameter();
            return CoreApi::ShapeCommandRegistry::instance().execute("CreateHyperbola", p);
        }, this);

    connect(m_session, &ShapePickSession::sessionCompleted, this, &DialogCreateHyperbola::onSessionCompleted);
    connect(m_session, &ShapePickSession::stateChanged, this, [this](ShapePickSession::State s) {
        if (s == ShapePickSession::State::Preview)
            m_statusLabel->setText(tr("Step 2/2 : Drag to set semi-axis a, then click"));
        else if (s == ShapePickSession::State::Picking)
            m_statusLabel->setText(tr("Step 1/2 : Click centre point in 3D view"));
    });
}

DialogCreateHyperbola::~DialogCreateHyperbola() {}

void DialogCreateHyperbola::show() { QDialog::show(); if (m_session) m_session->start(); }

void DialogCreateHyperbola::done(int result)
{
    if (m_session) m_session->stop();
    QDialog::done(result);
}

double DialogCreateHyperbola::centerX()     const { return m_spinBoxCenterX->value(); }
double DialogCreateHyperbola::centerY()     const { return m_spinBoxCenterY->value(); }
double DialogCreateHyperbola::centerZ()     const { return m_spinBoxCenterZ->value(); }
double DialogCreateHyperbola::normalX()     const { return m_spinBoxNormalX->value(); }
double DialogCreateHyperbola::normalY()     const { return m_spinBoxNormalY->value(); }
double DialogCreateHyperbola::normalZ()     const { return m_spinBoxNormalZ->value(); }
double DialogCreateHyperbola::majorRadius() const { return m_spinBoxMajorRadius->value(); }
double DialogCreateHyperbola::minorRadius() const { return m_spinBoxMinorRadius->value(); }
double DialogCreateHyperbola::firstParameter() const { return m_spinBoxFirstParameter->value(); }
double DialogCreateHyperbola::lastParameter() const { return m_spinBoxLastParameter->value(); }
QColor DialogCreateHyperbola::color()       const { return m_color; }

void DialogCreateHyperbola::closeEvent(QCloseEvent* event)
{
    if (m_session && m_session->isActive()) m_session->stop();
    QDialog::closeEvent(event);
}

void DialogCreateHyperbola::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, tr("Select Color"));
    if (c.isValid()) { m_color = c; m_btnColor->setStyleSheet(QString("background-color: %1").arg(c.name())); }
}

void DialogCreateHyperbola::onBtnOkClicked()
{
    if (firstParameter() >= lastParameter()
        || (normalX() == 0.0 && normalY() == 0.0 && normalZ() == 0.0)) {
        QMessageBox::warning(this, tr("Invalid Parameters"),
                             tr("The normal must be nonzero and the start parameter must be less than the end parameter."));
        return;
    }
    if (m_session) m_session->stop();
    emit signalCreateHyperbola(centerX(), centerY(), centerZ(),
                             normalX(), normalY(), normalZ(),
                             majorRadius(), minorRadius(), firstParameter(), lastParameter(), color());
}

void DialogCreateHyperbola::onSessionCompleted(QVector<gp_Pnt> points)
{
    if (points.size() < 2) return;
    const double major = points[0].Distance(points[1]);
    m_spinBoxCenterX->setValue(points[0].X());
    m_spinBoxCenterY->setValue(points[0].Y());
    m_spinBoxCenterZ->setValue(points[0].Z());
    if (major > 0.001) {
        m_spinBoxMajorRadius->setValue(major);
        m_spinBoxMinorRadius->setValue(major / 2.0);
    }
    m_statusLabel->setText(tr("Hyperbola defined. Press [Create] to confirm."));
}
