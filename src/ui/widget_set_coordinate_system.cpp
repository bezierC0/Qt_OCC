#include <QWindow>
#include <QMessageBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include <QCloseEvent>
#include <QtMath>

#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Face.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <gp_Trsf.hxx>
#include <gp_Dir.hxx>
#include <gp.hxx>
#include <gp_Pln.hxx>
#include <gp_Quaternion.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <StdSelect_BRepOwner.hxx>

#include "widget_set_coordinate_system.h"
#include "ui_widget_set_coordinate_system.h"
#include "OCCView.h"
#include "ViewManager.h"
#include "display/CoordinateSystemShape.h"

WidgetSetCoordinateSystem::WidgetSetCoordinateSystem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetSetCoordinateSystem),
    m_labelResultNormal(nullptr),
    m_previewCoordSys(nullptr)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint );

    // Change UI to Rotation mode
    ui->labelNormal->setText(tr("Rotation"));

    // Configure X Rotation
    ui->coordinateEditNormalX->setRange(-360.0, 360.0);
    ui->coordinateEditNormalX->setValue(0.0);
    ui->coordinateEditNormalX->setSingleStep(5.0);
    ui->coordinateEditNormalX->setSuffix(tr(" °"));
    ui->coordinateEditNormalX->setToolTip(tr("Rotation around X Axis"));

    // Configure Y Rotation
    ui->coordinateEditNormalY->setRange(-360.0, 360.0);
    ui->coordinateEditNormalY->setValue(0.0);
    ui->coordinateEditNormalY->setSingleStep(5.0);
    ui->coordinateEditNormalY->setSuffix(tr(" °"));
    ui->coordinateEditNormalY->setToolTip(tr("Rotation around Y Axis"));

    // Configure Z Rotation
    ui->coordinateEditNormalZ->setRange(-360.0, 360.0);
    ui->coordinateEditNormalZ->setValue(0.0);
    ui->coordinateEditNormalZ->setSingleStep(5.0);
    ui->coordinateEditNormalZ->setSuffix(tr(" °"));
    ui->coordinateEditNormalZ->setToolTip(tr("Rotation around Z Axis"));

    ui->gridLayout->addWidget(new QLabel(tr("Current Normal:"), this), 2, 0);
    
    m_labelResultNormal = new QLabel(this);
    m_labelResultNormal->setText(tr("(0.000, 0.000, 1.000)"));
    ui->gridLayout->addWidget(m_labelResultNormal, 2, 1, 1, 3);

    m_previewCoordSys = new CoordinateSystemShape();

    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &WidgetSetCoordinateSystem::onPushButtonCancel);
    connect(ui->pushButtonPickPoint, &QPushButton::clicked, this, &WidgetSetCoordinateSystem::onPickPointClicked);
    connect(ui->pushButtonPickNormal, &QPushButton::clicked, this, &WidgetSetCoordinateSystem::onPickNormalClicked);

    connect(ui->coordinateEditPointX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetSetCoordinateSystem::onCoordinateChanged);
    connect(ui->coordinateEditPointY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetSetCoordinateSystem::onCoordinateChanged);
    connect(ui->coordinateEditPointZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetSetCoordinateSystem::onCoordinateChanged);
    connect(ui->coordinateEditNormalX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetSetCoordinateSystem::onCoordinateChanged);
    connect(ui->coordinateEditNormalY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetSetCoordinateSystem::onCoordinateChanged);
    connect(ui->coordinateEditNormalZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetSetCoordinateSystem::onCoordinateChanged);
}

WidgetSetCoordinateSystem::~WidgetSetCoordinateSystem()
{
    if(m_previewCoordSys) {
        delete m_previewCoordSys;
        m_previewCoordSys = nullptr;
    }
    delete ui;
}

void WidgetSetCoordinateSystem::closeEvent(QCloseEvent *event)
{
    auto view = ViewManager::getInstance().getActiveView();
    if(view){
        if(m_previewCoordSys) {
            m_previewCoordSys->Remove(view->Context());
        }
        view->deactivateWorkPlane();
    }
    QWidget::closeEvent(event);
}
void WidgetSetCoordinateSystem::show()
{
    auto view = ViewManager::getInstance().getActiveView();
    if(view && m_previewCoordSys){
        m_previewCoordSys->Display(view->Context());
    }
    createWorkPlane();
    QWidget::show();
}

void WidgetSetCoordinateSystem::hide()
{
    auto view = ViewManager::getInstance().getActiveView();
    if(view){
        if(m_previewCoordSys) {
            m_previewCoordSys->Remove(view->Context());
        }
        view->deactivateWorkPlane();
        
        // Ensure to cancel any active pick session
        if (m_pickMode != PickMode::None) {
            restoreMouseState();
            m_pickMode = PickMode::None;
            disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetSetCoordinateSystem::onObjectSelected);
        }
    }
    QWidget::hide();
}

void WidgetSetCoordinateSystem::onPushButtonCancel()
{
    hide();
}

void WidgetSetCoordinateSystem::createWorkPlane()
{
    auto view = ViewManager::getInstance().getActiveView();
    if(!view){
        QMessageBox::warning(this, tr("Error"), tr("No active view"));
        return;
    }

    // Default normal is (0, 0, 1)
    gp_Dir normal(0.0, 0.0, 1.0);

    // Get input rotations in degrees
    double rx = ui->coordinateEditNormalX->value();
    double ry = ui->coordinateEditNormalY->value();
    double rz = ui->coordinateEditNormalZ->value();

    // Calculate transformation
    gp_Trsf trsf;
    gp_Trsf trsfX, trsfY, trsfZ;

    // Apply rotations X -> Y -> Z
    trsfX.SetRotation(gp::OX(), qDegreesToRadians(rx));
    trsfY.SetRotation(gp::OY(), qDegreesToRadians(ry));
    trsfZ.SetRotation(gp::OZ(), qDegreesToRadians(rz));

    trsf = trsfZ * trsfY * trsfX;

    // Transform the normal vector
    normal.Transform(trsf);

    // Update Preview Shape Location
    if(m_previewCoordSys) {
       gp_Dir xDir(1, 0, 0);
       xDir.Transform(trsf);

       gp_Pnt origin(ui->coordinateEditPointX->value(), 
                     ui->coordinateEditPointY->value(), 
                     ui->coordinateEditPointZ->value());
       
       m_previewCoordSys->SetLocation(gp_Ax2(origin, normal, xDir), view->Context());
    }

    // Update Display Label
    if(m_labelResultNormal){
        QString text = QString("(%1, %2, %3)")
                        .arg(normal.X(), 0, 'f', 3)
                        .arg(normal.Y(), 0, 'f', 3)
                        .arg(normal.Z(), 0, 'f', 3);
        m_labelResultNormal->setText(text);
    }

    view->createWorkPlane(ui->coordinateEditPointX->value(), 
                            ui->coordinateEditPointY->value(), 
                            ui->coordinateEditPointZ->value(),
                            normal.X(), 
                            normal.Y(), 
                            normal.Z());
}

void WidgetSetCoordinateSystem::onCoordinateChanged()
{
    createWorkPlane();
}

void WidgetSetCoordinateSystem::onPickPointClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    if (m_pickMode != PickMode::None) {
        // Reset previous pick if any
        restoreMouseState();
        disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetSetCoordinateSystem::onObjectSelected);
    }

    saveMouseState();
    m_pickMode = PickMode::Point;

    view->clearSelectedObjects();
    view->updateSelectionFilter(TopAbs_VERTEX, true);
    view->updateSelectionFilter(TopAbs_FACE, false); // Explicitly disable face
    view->setMouseMode(View::MouseMode::SELECTION);

    connect(view, &OCCView::signalSpaceSelected, this, &WidgetSetCoordinateSystem::onObjectSelected, Qt::UniqueConnection);
}

void WidgetSetCoordinateSystem::onPickNormalClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    if (m_pickMode != PickMode::None) {
        restoreMouseState();
        disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetSetCoordinateSystem::onObjectSelected);
    }

    saveMouseState();
    m_pickMode = PickMode::Face;

    view->clearSelectedObjects();
    view->updateSelectionFilter(TopAbs_FACE, true);
    view->updateSelectionFilter(TopAbs_VERTEX, false); // Explicitly disable vertex
    view->setMouseMode(View::MouseMode::SELECTION);
    
    connect(view, &OCCView::signalSpaceSelected, this, &WidgetSetCoordinateSystem::onObjectSelected, Qt::UniqueConnection);
}

void WidgetSetCoordinateSystem::onObjectSelected(const TopoDS_Shape& shape)
{
    if (m_pickMode == PickMode::None || shape.IsNull()) return;

    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    if (m_pickMode == PickMode::Point) {
        if (shape.ShapeType() == TopAbs_VERTEX) {
            TopoDS_Vertex v = TopoDS::Vertex(shape);
            gp_Pnt p = BRep_Tool::Pnt(v);
            
            ui->coordinateEditPointX->setValue(p.X());
            ui->coordinateEditPointY->setValue(p.Y());
            ui->coordinateEditPointZ->setValue(p.Z());
        }
    } else if (m_pickMode == PickMode::Face) {
        if (shape.ShapeType() == TopAbs_FACE) {
            TopoDS_Face f = TopoDS::Face(shape);
            BRepAdaptor_Surface surf(f);
            if (surf.GetType() == GeomAbs_Plane) {
                gp_Pln plane = surf.Plane();
                gp_Dir normal = plane.Axis().Direction();
                if (f.Orientation() == TopAbs_REVERSED) normal.Reverse();

                gp_Quaternion q;
                q.SetRotation(gp_Vec(0, 0, 1), gp_Vec(normal.XYZ())); // Use Z-axis vector
                double rx, ry, rz;
                q.GetEulerAngles(gp_Intrinsic_ZYX, rz, ry, rx);
                
                ui->coordinateEditNormalX->setValue(qRadiansToDegrees(rx));
                ui->coordinateEditNormalY->setValue(qRadiansToDegrees(ry));
                ui->coordinateEditNormalZ->setValue(qRadiansToDegrees(rz));
            }
        }
    }

    // Cleanup and end pick session
    restoreMouseState();
    m_pickMode = PickMode::None;
    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetSetCoordinateSystem::onObjectSelected);
}

void WidgetSetCoordinateSystem::saveMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters = view->getSelectionFilters();
}

void WidgetSetCoordinateSystem::restoreMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    
    view->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
    view->clearSelectedObjects();
    
    // Restore filters
    for(const auto& filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, filter.second);
    }
}

