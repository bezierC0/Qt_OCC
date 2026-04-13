#include "widget_minimum_distance.h"
#include "ui_widget_minimum_distance.h"
#include "ViewManager.h"
#include "OCCView.h"
#include "SelectedEntity.h"

#include <AIS_Shape.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepExtrema_ProximityDistTool.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include "util/TopoShapeUtil.h"
#include "common/ShapeLabelManager.h"
#include <TDataStd_Name.hxx>

#include <QtMath>
#include <QMessageBox>
#include <QDebug>

WidgetMinimumDistance::WidgetMinimumDistance(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetMinimumDistance),
    m_pickingState(Idle),
    m_hasP1(false),
    m_hasP2(false)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    connect(ui->pushButtonPick, &QPushButton::clicked, this, &WidgetMinimumDistance::onPickClicked);
    connect(ui->pushButtonApply, &QPushButton::clicked, this, &WidgetMinimumDistance::onApplyClicked);
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &WidgetMinimumDistance::onCloseClicked);
}

WidgetMinimumDistance::~WidgetMinimumDistance()
{
    delete ui;
}

void WidgetMinimumDistance::show()
{
    QWidget::show();
    onPickClicked();
}

void WidgetMinimumDistance::hide()
{
    clearResultDisplay();
    if (m_pickingState != Idle) {
        restoreMouseState();
    }
    QWidget::hide();
}

void WidgetMinimumDistance::closeEvent(QCloseEvent *event)
{
    clearResultDisplay();
    if (m_pickingState != Idle) {
        restoreMouseState();
    }
    QWidget::closeEvent(event);
}

void WidgetMinimumDistance::onPickClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    if (m_pickingState != Idle) {
        restoreMouseState();
    }

    saveMouseState();
    m_pickingState = PickFirst;
    m_hasP1 = false;
    m_hasP2 = false;
    m_shape1.Nullify();
    m_shape2.Nullify();
    
    clearResultDisplay();

    view->clearSelectedObjects();
    view->updateSelectionFilter(TopAbs_VERTEX, true);
    view->updateSelectionFilter(TopAbs_EDGE, true);
    view->updateSelectionFilter(TopAbs_FACE, true);
    
    view->setMouseMode(View::MouseMode::SELECTION);
    
    connect(view, &OCCView::signalSpaceSelected, this, &WidgetMinimumDistance::onObjectSelected);
    
    updateUI();
}

QString WidgetMinimumDistance::getShapeNameAndType(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) return "Not Selected";
    
    QString displayName = QString::fromStdString(Util::TopoShape::GetShapeTypeString(shape));
    TDF_Label label = ShapeLabelManager::GetInstance().GetLabel(shape);
    if (!label.IsNull()) {
        Handle(TDataStd_Name) name;
        if (label.FindAttribute(TDataStd_Name::GetID(), name)) {
            TCollection_ExtendedString extStr = name->Get();
            displayName = QString("%1: %2").arg(displayName).arg(QString::fromUtf16(reinterpret_cast<const ushort*>(extStr.ToExtString())));
        }
    }
    return displayName;
}

void WidgetMinimumDistance::onObjectSelected(const TopoDS_Shape& shape)
{
    if (m_pickingState == Idle || shape.IsNull()) return;
    auto type = shape.ShapeType();
    if (type != TopAbs_VERTEX && type != TopAbs_EDGE && type != TopAbs_FACE) return;

    if (m_pickingState == PickFirst) {
        m_shape1 = shape;
        m_hasP1 = true;
        m_pickingState = PickSecond;
        updateUI();
    }
    else if (m_pickingState == PickSecond) {
        m_shape2 = shape;
        m_hasP2 = true;
        m_pickingState = Idle; // Done picking
        updateUI();
        
        restoreMouseState();
    }
}

void WidgetMinimumDistance::onApplyClicked()
{
    calculateMinimumDistance();
}

void WidgetMinimumDistance::calculateMinimumDistance()
{
    if (!m_hasP1 || !m_hasP2 || m_shape1.IsNull() || m_shape2.IsNull()) return;

    clearResultDisplay();

    // BRepExtrema_ProximityDistTool transform mesh (Poly_Triangulation) calculate distance
    /*
    BRepExtrema_ShapeProximity proximity(shape1, shape2);
    proximity.Perform();
    Standard_Real proxDist = proximity.Proximity();  
    */
    BRepExtrema_DistShapeShape extrema(m_shape1, m_shape2);
    if (extrema.IsDone() && extrema.NbSolution() > 0) {
        const double precision = 4;
        double dist = extrema.Value();
        ui->labelDistanceValue->setText(QString::number(dist, 'f', precision));

        gp_Pnt p1 = extrema.PointOnShape1(1);
        gp_Pnt p2 = extrema.PointOnShape2(1);

        auto view = ViewManager::getInstance().getActiveView();
        if (view) {
            auto context = view->Context();
            if (!context.IsNull()) {
                BRepBuilderAPI_MakeVertex makeV1(p1);
                BRepBuilderAPI_MakeVertex makeV2(p2);
                
                Handle(AIS_Shape) aisV1 = new AIS_Shape(makeV1.Shape());
                aisV1->SetColor(Quantity_NOC_RED);
                aisV1->SetWidth(5.0); 

                Handle(AIS_Shape) aisV2 = new AIS_Shape(makeV2.Shape());
                aisV2->SetColor(Quantity_NOC_RED);
                aisV2->SetWidth(5.0);

                m_resultObjects.push_back(aisV1);
                m_resultObjects.push_back(aisV2);
                
                context->Display(aisV1, Standard_False);
                context->Display(aisV2, Standard_False);

                if (p1.Distance(p2) > 1e-6) {
                    BRepBuilderAPI_MakeEdge makeEdge(p1, p2);
                    if (makeEdge.IsDone()) {
                        Handle(AIS_Shape) aisEdge = new AIS_Shape(makeEdge.Shape());
                        aisEdge->SetColor(Quantity_NOC_GREEN);
                        aisEdge->SetWidth(2.0);
                        m_resultObjects.push_back(aisEdge);
                        context->Display(aisEdge, Standard_False);

                        Handle(AIS_TextLabel) aTextLabel = new AIS_TextLabel();
                        aTextLabel->SetText(std::to_string(p1.Distance(p2)).c_str());
                        aTextLabel->SetPosition({(p1.X() + p2.X())/2,(p1.Y() + p2.Y())/2,(p1.Z() + p2.Z())/2});
                        m_resultObjects.push_back(aTextLabel);
                        context->Display(aTextLabel, Standard_False);
                    }
                }
                view->reDraw();
            }
        }
    } else {
        ui->labelDistanceValue->setText("N/A");
        QMessageBox::warning(this, "Minimum Distance", "Failed to calculate the minimum distance between the two shapes.");
    }
}

void WidgetMinimumDistance::clearResultDisplay()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (view && !m_resultObjects.empty()) {
        auto context = view->Context();
        if (!context.IsNull()) {
            for (const auto& obj : m_resultObjects) {
                context->Remove(obj, Standard_False);
            }
            view->reDraw();
        }
        m_resultObjects.clear();
    }
}

void WidgetMinimumDistance::updateUI()
{
    if (m_hasP1 && !m_shape1.IsNull()) {
        ui->labelObj1Value->setText(getShapeNameAndType(m_shape1));
    } else {
        ui->labelObj1Value->setText("Not Selected");
    }

    if (m_hasP2 && !m_shape2.IsNull()) {
        ui->labelObj2Value->setText(getShapeNameAndType(m_shape2));
    } else {
        ui->labelObj2Value->setText("Not Selected");
    }

    if (m_pickingState == PickFirst) {
        ui->labelStatus->setText("Please select the first object.");
    } else if (m_pickingState == PickSecond) {
        ui->labelStatus->setText("Please select the second object.");
    } else {
        ui->labelStatus->setText("Measurement ready. Click Apply.");
    }
    
    if (!m_hasP1 || !m_hasP2) {
         ui->labelDistanceValue->setText("0.00");
    }

    ui->pushButtonApply->setEnabled(m_hasP1 && m_hasP2);
}

void WidgetMinimumDistance::onCloseClicked()
{
    close();
}

void WidgetMinimumDistance::saveMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters = view->getSelectionFilters();
}

void WidgetMinimumDistance::restoreMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    
    // Disconnect signals
    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetMinimumDistance::onObjectSelected);

    view->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
    
    for(const auto& filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, filter.second);
    }
    m_pickingState = Idle;
}
