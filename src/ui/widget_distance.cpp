#include "widget_distance.h"
#include "ui_widget_distance.h"
#include "ViewManager.h"
#include "OCCView.h"
#include "SelectedEntity.h"
#include "SelectionPickSession.h"

#include <AIS_Shape.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>

#include <QtMath>
#include <QMessageBox>
#include <QDebug>

WidgetDistance::WidgetDistance(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetDistance),
    m_pickSession(new SelectionPickSession(this)),
    m_pickingState(Idle),
    m_hasP1(false),
    m_hasP2(false)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    connect(ui->pushButtonPick1, &QPushButton::clicked, this, &WidgetDistance::onPickFirstClicked);
    connect(ui->pushButtonPick2, &QPushButton::clicked, this, &WidgetDistance::onPickSecondClicked);
    connect(ui->pushButtonClear1, &QPushButton::clicked, this, &WidgetDistance::onClearFirstClicked);
    connect(ui->pushButtonClear2, &QPushButton::clicked, this, &WidgetDistance::onClearSecondClicked);
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &WidgetDistance::onCloseClicked);
    connect(m_pickSession, &SelectionPickSession::shapePicked,
            this, &WidgetDistance::onObjectSelected);
}

WidgetDistance::~WidgetDistance()
{
    delete ui;
}

void WidgetDistance::show()
{
    QWidget::show();
    
    // Check current selection if we can pre-fill
    auto view = ViewManager::getInstance().getActiveView();
    if (view) {
        const auto &selectedList = view->getSelectedObjects();
        if (selectedList.size() == 2) {
            auto ent1 = selectedList.at(0);
            auto ent2 = selectedList.at(1);
            if (ent1 && ent2) {
               auto aisShape1 = ent1->GetSelectedShape();
               auto aisShape2 = ent2->GetSelectedShape();
               if (!aisShape1.IsNull() && !aisShape2.IsNull()) {
                   TopoDS_Shape shape1 = aisShape1->Shape();
                   TopoDS_Shape shape2 = aisShape2->Shape();
                   if (shape1.ShapeType() == TopAbs_VERTEX && shape2.ShapeType() == TopAbs_VERTEX) {
                        m_pnt1 = BRep_Tool::Pnt(TopoDS::Vertex(shape1));
                        m_pnt2 = BRep_Tool::Pnt(TopoDS::Vertex(shape2));
                        m_hasP1 = true;
                        m_hasP2 = true;
                        updateUI();
                        calculateDistance();
                        return;
                   }
               }
            }
        }
    }

    // Default: Start picking if not enough valid selection
    m_hasP1 = false;
    m_hasP2 = false;
    startPicking(PickFirst);
}

void WidgetDistance::hide()
{
    if (m_pickingState != Idle) {
        restoreMouseState();
    }
    QWidget::hide();
}

void WidgetDistance::closeEvent(QCloseEvent *event)
{
    if (m_pickingState != Idle) {
        restoreMouseState();
    }
    QWidget::closeEvent(event);
}

void WidgetDistance::onPickFirstClicked()
{
    startPicking(PickFirst);
}

void WidgetDistance::onPickSecondClicked()
{
    startPicking(PickSecond);
}

void WidgetDistance::onClearFirstClicked()
{
    if (m_pickingState == PickFirst) restoreMouseState();
    m_hasP1 = false;
    updateUI();
}

void WidgetDistance::onClearSecondClicked()
{
    if (m_pickingState == PickSecond) restoreMouseState();
    m_hasP2 = false;
    updateUI();
}

void WidgetDistance::startPicking(PickingState target)
{
    if (m_pickingState != Idle) restoreMouseState();
    if (!ViewManager::getInstance().getActiveView()) return;

    if (!m_pickSession->start({{TopAbs_VERTEX}})) return;
    m_pickingState = target;
    if (target == PickFirst) m_hasP1 = false;
    else m_hasP2 = false;
    updateUI();
}

void WidgetDistance::onObjectSelected(const TopoDS_Shape& shape)
{
    if (m_pickingState == Idle || shape.IsNull() || shape.ShapeType() != TopAbs_VERTEX) return;

    gp_Pnt p = BRep_Tool::Pnt(TopoDS::Vertex(shape));

    if (m_pickingState == PickFirst) {
        m_pnt1 = p;
        m_hasP1 = true;
    } else {
        m_pnt2 = p;
        m_hasP2 = true;
    }

    restoreMouseState();
    updateUI();
    calculateDistance();
}

void WidgetDistance::calculateDistance()
{
    const double precision = 4;
    if (m_hasP1 && m_hasP2) {
        double dist = m_pnt1.Distance(m_pnt2);
        ui->labelDistanceValue->setText(QString::number(dist, 'f', precision));

        double dx = qAbs(m_pnt1.X() - m_pnt2.X());
        double dy = qAbs(m_pnt1.Y() - m_pnt2.Y());
        double dz = qAbs(m_pnt1.Z() - m_pnt2.Z());

        ui->labelDeltaXValue->setText(QString::number(dx, 'f', precision));
        ui->labelDeltaYValue->setText(QString::number(dy, 'f', precision));
        ui->labelDeltaZValue->setText(QString::number(dz, 'f', precision));
    }
}

void WidgetDistance::updateUI()
{
    if (m_hasP1) {
        ui->labelPoint1Value->setText(QString("X: %1, Y: %2, Z: %3")
            .arg(m_pnt1.X(), 0, 'f', 2)
            .arg(m_pnt1.Y(), 0, 'f', 2)
            .arg(m_pnt1.Z(), 0, 'f', 2));
    } else {
        ui->labelPoint1Value->setText(tr("Not Selected"));
    }

    if (m_hasP2) {
        ui->labelPoint2Value->setText(QString("X: %1, Y: %2, Z: %3")
            .arg(m_pnt2.X(), 0, 'f', 2)
            .arg(m_pnt2.Y(), 0, 'f', 2)
            .arg(m_pnt2.Z(), 0, 'f', 2));
    } else {
        ui->labelPoint2Value->setText(tr("Not Selected"));
    }

    if (m_pickingState == PickFirst) {
        ui->labelStatus->setText(tr("Please select the first vertex."));
    } else if (m_pickingState == PickSecond) {
        ui->labelStatus->setText(tr("Please select the second vertex."));
    } else if (m_hasP1 && m_hasP2) {
        ui->labelStatus->setText(tr("Measurement complete."));
    } else {
        ui->labelStatus->setText(tr("Pick a point to measure distance."));
    }
    
    if (!m_hasP1 || !m_hasP2) {
         ui->labelDistanceValue->setText("0.00");
         ui->labelDeltaXValue->setText("0.00");
         ui->labelDeltaYValue->setText("0.00");
         ui->labelDeltaZValue->setText("0.00");
    }
}

void WidgetDistance::onCloseClicked()
{
    close();
}

void WidgetDistance::restoreMouseState()
{
    m_pickSession->stop();
    m_pickingState = Idle;
}
