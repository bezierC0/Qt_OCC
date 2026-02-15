#include "widget_transform.h"
#include "ui_widget_transform.h"
#include "ViewManager.h"
#include "OCCView.h"
#include "SelectedEntity.h"
#include "TopoShapeUtil.h"

#include <AIS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>
#include <BRep_Tool.hxx>
#include <gp_Trsf.hxx>
#include <gp_Quaternion.hxx>


#include <QtMath>
#include <QMessageBox>
#include <QDebug>

WidgetTransform::WidgetTransform(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetTransform),
    m_isPicking(false),
    m_targetObject(nullptr)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint);

    connect(ui->pushButtonPick, &QPushButton::clicked, this, &WidgetTransform::onPickClicked);
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &WidgetTransform::onCloseClicked);

    // Connect 
    connect(ui->spinBoxPosX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetTransform::onTransformChanged);
    connect(ui->spinBoxPosY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetTransform::onTransformChanged);
    connect(ui->spinBoxPosZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetTransform::onTransformChanged);
    connect(ui->spinBoxRotX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetTransform::onTransformChanged);
    connect(ui->spinBoxRotY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetTransform::onTransformChanged);
    connect(ui->spinBoxRotZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetTransform::onTransformChanged);
}

WidgetTransform::~WidgetTransform()
{
    delete ui;
}

void WidgetTransform::show()
{
    QWidget::show();
    
    // Subscribe
    if (auto view = ViewManager::getInstance().getActiveView()) {
        view->addManipulatorObserver(this);
    }

    if (m_targetObject.IsNull()) {
        onPickClicked();
    }
}

void WidgetTransform::hide()
{
    restoreMouseState();
    auto view = ViewManager::getInstance().getActiveView();
    if (view) {
         view->detachManipulator();
         view->removeManipulatorObserver(this);
    }
    QWidget::hide();
}

void WidgetTransform::closeEvent(QCloseEvent *event)
{
    restoreMouseState();
    if (m_targetObject) {
        // Optionally detach manipulator here if we were using one
        auto view = ViewManager::getInstance().getActiveView();
        if (view) {
             view->detachManipulator();
        }
    }
    QWidget::closeEvent(event);
}

void WidgetTransform::onPickClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    if (m_isPicking) {
        restoreMouseState();
    }

    saveMouseState();
    m_isPicking = true;

    view->clearSelectedObjects();
    view->updateSelectionFilter(TopAbs_SOLID, true);
    view->setMouseMode(View::MouseMode::SELECTION);
    
    // Disconnect old connection if any to avoid duplicates
    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetTransform::onObjectSelected);
    // disconnect(view, &OCCView::signalManipulatorChange, this, &WidgetTransform::onManipulatorChanged);

    connect(view, &OCCView::signalSpaceSelected, this, &WidgetTransform::onObjectSelected);
}

void WidgetTransform::onObjectSelected(const TopoDS_Shape& shape)
{
    if (!m_isPicking || shape.IsNull()) return;

    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    // Find the AIS Object Check selected objects in view
    const auto selectedObjects = view->getSelectedObjects();
    if (selectedObjects.empty()) return;

    // Use the first selected object's parent interactive object
    m_targetObject = selectedObjects.at(0)->GetParentInteractiveObject();
    m_targetShape = shape;

    // Update UI name
    TCollection_ExtendedString name = Util::Ais::GetNameFromAISObject(m_targetObject);
    QString objectName;
    if (name.IsEmpty()) {
        objectName = tr("Selected Solid");
    } else {
        objectName = QString::fromUtf16(name.ToExtString());
    }
    ui->labelObjectName->setText(objectName);

    // Get current transformation
    gp_Trsf trsf = m_targetObject->LocalTransformation();
    gp_XYZ loc = trsf.TranslationPart();
    gp_Quaternion rot = trsf.GetRotation();
    
    double rx, ry, rz;
    rot.GetEulerAngles(gp_Intrinsic_ZYX, rz, ry, rx);

    // Block signals to prevent triggering updateTransform
    bool oldState = ui->spinBoxPosX->blockSignals(true);
    ui->spinBoxPosY->blockSignals(true);
    ui->spinBoxPosZ->blockSignals(true);
    ui->spinBoxRotX->blockSignals(true);
    ui->spinBoxRotY->blockSignals(true);
    ui->spinBoxRotZ->blockSignals(true);

    ui->spinBoxPosX->setValue(loc.X());
    ui->spinBoxPosY->setValue(loc.Y());
    ui->spinBoxPosZ->setValue(loc.Z());
    ui->spinBoxRotX->setValue(qRadiansToDegrees(rx));
    ui->spinBoxRotY->setValue(qRadiansToDegrees(ry));
    ui->spinBoxRotZ->setValue(qRadiansToDegrees(rz));

    ui->spinBoxPosX->blockSignals(oldState);
    ui->spinBoxPosY->blockSignals(oldState);
    ui->spinBoxPosZ->blockSignals(oldState);
    ui->spinBoxRotX->blockSignals(oldState);
    ui->spinBoxRotY->blockSignals(oldState);
    ui->spinBoxRotZ->blockSignals(oldState);

    // Restore mouse state
    m_isPicking = false;
    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetTransform::onObjectSelected);

    // connect(view, &OCCView::signalManipulatorChange, this, &WidgetTransform::onManipulatorChanged); // Replaced by Observer

    // Attach manipulator for visual feedback (optional, but requested by implication of 'transform')
    view->attachManipulator(m_targetObject);
    view->reDraw();
    
    // view->setMouseMode(View::MANIPULATE); // Removed: Transient mode setting in View is preferred
}

void WidgetTransform::onTransformChanged()
{
    if (m_targetObject.IsNull()) return;
    updateTransform();
}

void WidgetTransform::updateTransform()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    const double x = ui->spinBoxPosX->value();
    const double y = ui->spinBoxPosY->value();
    const double z = ui->spinBoxPosZ->value();
    
    const double rx = qDegreesToRadians(ui->spinBoxRotX->value());
    const double ry = qDegreesToRadians(ui->spinBoxRotY->value());
    const double rz = qDegreesToRadians(ui->spinBoxRotZ->value());

    gp_Trsf trsf;
    gp_Quaternion q;
    q.SetEulerAngles(gp_Intrinsic_ZYX, rz, ry, rx);
    
    trsf.SetRotation(q);
    trsf.SetTranslationPart(gp_Vec(x, y, z));

    m_targetObject->SetLocalTransformation(trsf);

    // Bug 2 fix: Update manipulator position
    view->updateManipulator(); // This method we added to OCCView

    // view->attachManipulator(m_targetObject); 

    view->reDraw();
}

void WidgetTransform::onResetClicked()
{
    // TODO: Reset transform to identity?
}

void WidgetTransform::onCloseClicked()
{
    close();
}

void WidgetTransform::saveMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters = view->getSelectionFilters();
}

void WidgetTransform::restoreMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    
    view->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
    view->clearSelectedObjects();
    
    for(const auto& filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, filter.second);
    }

    //disconnect(view, &OCCView::signalManipulatorChange, this, &WidgetTransform::onManipulatorChanged);
}

void WidgetTransform::onManipulatorChange(const gp_Trsf& trsf)
{
    // Update UI from trsf
    gp_XYZ loc = trsf.TranslationPart();
    gp_Quaternion rot = trsf.GetRotation();
    double rx, ry, rz;
    rot.GetEulerAngles(gp_Intrinsic_ZYX, rz, ry, rx);
    
    // Block signals
    bool oldState = ui->spinBoxPosX->blockSignals(true);
    ui->spinBoxPosY->blockSignals(true);
    ui->spinBoxPosZ->blockSignals(true);
    ui->spinBoxRotX->blockSignals(true);
    ui->spinBoxRotY->blockSignals(true);
    ui->spinBoxRotZ->blockSignals(true);

    ui->spinBoxPosX->setValue(loc.X());
    ui->spinBoxPosY->setValue(loc.Y());
    ui->spinBoxPosZ->setValue(loc.Z());
    ui->spinBoxRotX->setValue(qRadiansToDegrees(rx));
    ui->spinBoxRotY->setValue(qRadiansToDegrees(ry));
    ui->spinBoxRotZ->setValue(qRadiansToDegrees(rz));

    ui->spinBoxPosX->blockSignals(oldState);
    ui->spinBoxPosY->blockSignals(oldState);
    ui->spinBoxPosZ->blockSignals(oldState);
    ui->spinBoxRotX->blockSignals(oldState);
    ui->spinBoxRotY->blockSignals(oldState);
    ui->spinBoxRotZ->blockSignals(oldState);
}
