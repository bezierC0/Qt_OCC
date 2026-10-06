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
#include <TopLoc_Location.hxx>


#include <QtMath>
#include <QMessageBox>
#include <QDebug>
#include <algorithm>

WidgetTransform::WidgetTransform(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetTransform),
    m_isPicking(false),
    m_targetObject(nullptr)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint);

    connect(ui->pushButtonPick, &QPushButton::clicked, this, &WidgetTransform::onPickClicked);
    connect(ui->pushButtonApply, &QPushButton::clicked, this, &WidgetTransform::onApplyClicked);
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
    finishInteraction();
    clearTarget();
    QWidget::show();

    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    view->addManipulatorObserver(this);
    onPickClicked();
}

void WidgetTransform::hide()
{
    finishInteraction();
    QWidget::hide();
}

void WidgetTransform::closeEvent(QCloseEvent *event)
{
    finishInteraction();
    QWidget::closeEvent(event);
}

void WidgetTransform::onPickClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    restorePreview();
    clearTarget();

    if (m_isPicking) {
        restoreMouseState();
    }

    saveMouseState();
    m_isPicking = true;

    view->detachManipulator();
    view->clearSelectedObjects();
    for (const auto& filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, filter.first == TopAbs_SOLID);
    }
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

    const auto selectedIt = std::find_if(
        selectedObjects.cbegin(), selectedObjects.cend(),
        [&shape](const std::shared_ptr<View::SelectedEntity>& entity) {
            return entity && !entity->GetSelectedShape().IsNull()
                && shape.IsSame(entity->GetSelectedShape()->Shape());
        });
    if (selectedIt == selectedObjects.cend()) return;

    m_targetObject = (*selectedIt)->GetParentInteractiveObject();
    if (m_targetObject.IsNull()) return;
    m_targetShape = shape;
    m_originalTransform = m_targetObject->LocalTransformation();
    m_hasOriginalTransform = true;
    ui->pushButtonApply->setEnabled(true);

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

    m_isPicking = false;
    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetTransform::onObjectSelected);
    restoreMouseState();

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

    view->Context()->SetLocation(m_targetObject, TopLoc_Location(trsf));

    // Bug 2 fix: Update manipulator position
    view->updateManipulator(); // This method we added to OCCView

    // view->attachManipulator(m_targetObject); 

    view->requestSceneRedraw();
    view->repaint();
}

void WidgetTransform::onResetClicked()
{
    // TODO: Reset transform to identity?
}

void WidgetTransform::onCloseClicked()
{
    close();
}

void WidgetTransform::onApplyClicked()
{
    if (m_targetObject.IsNull()) return;

    m_originalTransform = m_targetObject->LocalTransformation();
    m_hasOriginalTransform = true;
    if (auto view = ViewManager::getInstance().getActiveView()) {
        view->Context()->SetLocation(m_targetObject, TopLoc_Location(m_originalTransform));
        view->requestSceneRedraw();
        view->repaint();
    }
}

void WidgetTransform::saveMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view || m_hasSavedMouseState) return;
    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters = view->getSelectionFilters();
    m_hasSavedMouseState = true;
}

void WidgetTransform::restoreMouseState()
{
    if (!m_hasSavedMouseState) return;
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        m_hasSavedMouseState = false;
        return;
    }
    
    view->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
    view->clearSelectedObjects();
    for (const auto& filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, filter.second);
    }
    m_hasSavedMouseState = false;
}

void WidgetTransform::finishInteraction()
{
    m_isPicking = false;
    restorePreview();
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        m_hasSavedMouseState = false;
        clearTarget();
        return;
    }

    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetTransform::onObjectSelected);
    restoreMouseState();
    view->detachManipulator();
    view->removeManipulatorObserver(this);
    clearTarget();
}

void WidgetTransform::restorePreview()
{
    if (m_targetObject.IsNull() || !m_hasOriginalTransform) return;

    if (auto view = ViewManager::getInstance().getActiveView()) {
        view->Context()->SetLocation(m_targetObject, TopLoc_Location(m_originalTransform));
        view->requestSceneRedraw();
        view->repaint();
    } else {
        m_targetObject->SetLocalTransformation(m_originalTransform);
    }
}

void WidgetTransform::clearTarget()
{
    m_targetObject.Nullify();
    m_targetShape.Nullify();
    m_hasOriginalTransform = false;
    ui->labelObjectName->setText(tr("None"));
    ui->pushButtonApply->setEnabled(false);

    QDoubleSpinBox* editors[] = {
        ui->spinBoxPosX, ui->spinBoxPosY, ui->spinBoxPosZ,
        ui->spinBoxRotX, ui->spinBoxRotY, ui->spinBoxRotZ
    };
    for (QDoubleSpinBox* editor : editors) {
        const bool oldState = editor->blockSignals(true);
        editor->setValue(0.0);
        editor->blockSignals(oldState);
    }
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
