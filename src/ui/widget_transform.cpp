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
#include <TDocStd_Document.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFPrs_AISObject.hxx>


#include <QtMath>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDebug>
#include <QSignalBlocker>
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
    connect(ui->pushButtonReset, &QPushButton::clicked, this, &WidgetTransform::onResetClicked);
    connect(ui->pushButtonApply, &QPushButton::clicked, this, &WidgetTransform::onApplyClicked);
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &WidgetTransform::onCloseClicked);
    connect(&ViewManager::getInstance(), &ViewManager::activeViewChanged,
            this, &WidgetTransform::onActiveViewChanged);

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
    finishInteraction();
    delete ui;
}

void WidgetTransform::show()
{
    finishInteraction();
    clearTarget();
    QWidget::show();

    m_view = ViewManager::getInstance().getActiveView();
    if (!m_view) return;

    m_view->addManipulatorObserver(this);
    connect(m_view, &OCCView::signalShapeObjectsChanged,
            this, &WidgetTransform::onShapeObjectsChanged, Qt::UniqueConnection);
    connect(m_view, &OCCView::signalEscapePressed,
            this, &WidgetTransform::onCancelRequested, Qt::UniqueConnection);
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

void WidgetTransform::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void WidgetTransform::onPickClicked()
{
    OCCView* view = m_view.data();
    if (!view) return;

    restorePreview();
    clearTarget();
    ui->labelStatus->setText(tr("Select an object in the view."));

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

    OCCView* view = m_view.data();
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
    m_targetLabel.Nullify();
    const Handle(XCAFPrs_AISObject) xcafObject =
        Handle(XCAFPrs_AISObject)::DownCast(m_targetObject);
    if (!xcafObject.IsNull()) {
        m_targetLabel = xcafObject->GetLabel();
        m_targetLocation = XCAFDoc_ShapeTool::GetLocation(m_targetLabel);
    } else {
        m_targetLocation = TopLoc_Location();
    }
    m_parentTransform = m_targetObject->LocalTransformation();
    m_initialTransform = m_parentTransform;
    m_initialTransform.Multiply(m_targetLocation.Transformation());
    m_originalTransform = m_initialTransform;
    m_workingTransform = m_originalTransform;
    m_hasOriginalTransform = true;
    ui->pushButtonReset->setEnabled(true);
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
    ui->labelStatus->setText(tr("Ready."));

    updateEditorsFromTransform(m_workingTransform);

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
    OCCView* view = m_view.data();
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

    applyWorkingTransform(trsf, TransformSource::Editors);
}

void WidgetTransform::applyWorkingTransform(const gp_Trsf& transform, TransformSource source)
{
    OCCView* view = m_view.data();
    if (!view || m_targetObject.IsNull()) return;

    m_workingTransform = transform;
    gp_Trsf previewTransform = m_workingTransform;
    previewTransform.Multiply(m_targetLocation.Transformation().Inverted());
    view->Context()->SetLocation(m_targetObject, TopLoc_Location(previewTransform));

    if (source == TransformSource::Editors) {
        view->updateManipulator();
        ui->labelStatus->setText(tr("Previewing changes."));
    } else if (source == TransformSource::Manipulator) {
        updateEditorsFromTransform(m_workingTransform);
        ui->labelStatus->setText(tr("Previewing changes."));
    } else if (source == TransformSource::Reset) {
        updateEditorsFromTransform(m_workingTransform);
        view->updateManipulator();
        ui->labelStatus->setText(tr("Previewing original transform."));
    }

    view->requestSceneRedraw();
    view->repaint();
}

void WidgetTransform::updateEditorsFromTransform(const gp_Trsf& transform)
{
    const gp_XYZ location = transform.TranslationPart();
    const gp_Quaternion rotation = transform.GetRotation();
    double rx, ry, rz;
    rotation.GetEulerAngles(gp_Intrinsic_ZYX, rz, ry, rx);

    const QSignalBlocker blockPosX(ui->spinBoxPosX);
    const QSignalBlocker blockPosY(ui->spinBoxPosY);
    const QSignalBlocker blockPosZ(ui->spinBoxPosZ);
    const QSignalBlocker blockRotX(ui->spinBoxRotX);
    const QSignalBlocker blockRotY(ui->spinBoxRotY);
    const QSignalBlocker blockRotZ(ui->spinBoxRotZ);

    ui->spinBoxPosX->setValue(location.X());
    ui->spinBoxPosY->setValue(location.Y());
    ui->spinBoxPosZ->setValue(location.Z());
    ui->spinBoxRotX->setValue(qRadiansToDegrees(rx));
    ui->spinBoxRotY->setValue(qRadiansToDegrees(ry));
    ui->spinBoxRotZ->setValue(qRadiansToDegrees(rz));
}

void WidgetTransform::onResetClicked()
{
    if (m_targetObject.IsNull() || !m_hasOriginalTransform) return;
    applyWorkingTransform(m_initialTransform, TransformSource::Reset);
}

void WidgetTransform::onCloseClicked()
{
    close();
}

void WidgetTransform::onApplyClicked()
{
    if (m_targetObject.IsNull()) return;

    if (!commitWorkingTransform()) {
        ui->labelStatus->setText(tr("Failed to apply transform."));
        return;
    }

    m_originalTransform = m_workingTransform;
    m_hasOriginalTransform = true;
    ui->labelStatus->setText(tr("Changes applied."));
}

bool WidgetTransform::commitWorkingTransform()
{
    if (m_targetLabel.IsNull()) return true;

    Handle(TDocStd_Document) document = TDocStd_Document::Get(m_targetLabel);
    if (document.IsNull()) return false;

    Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(document->Main());
    if (shapeTool.IsNull()) return false;

    gp_Trsf targetLocation = m_parentTransform.Inverted();
    targetLocation.Multiply(m_workingTransform);

    const bool openedCommand = !document->HasOpenCommand();
    if (openedCommand) document->OpenCommand();

    TDF_Label updatedLabel = m_targetLabel;
    bool locationUpdated = false;
    if (XCAFDoc_ShapeTool::IsSimpleShape(m_targetLabel)
        && XCAFDoc_ShapeTool::IsFree(m_targetLabel)) {
        TopoDS_Shape shape;
        if (XCAFDoc_ShapeTool::GetShape(m_targetLabel, shape)) {
            shape.Location(TopLoc_Location(targetLocation));
            shapeTool->SetShape(m_targetLabel, shape);
            locationUpdated = true;
        }
    } else {
        gp_Trsf locationDelta = targetLocation;
        locationDelta.Multiply(m_targetLocation.Transformation().Inverted());
        locationUpdated = shapeTool->SetLocation(
            m_targetLabel, TopLoc_Location(locationDelta), updatedLabel);
    }

    if (!locationUpdated) {
        if (openedCommand) document->AbortCommand();
        return false;
    }

    const Handle(XCAFPrs_AISObject) xcafObject =
        Handle(XCAFPrs_AISObject)::DownCast(m_targetObject);
    if (!xcafObject.IsNull()) {
        xcafObject->SetLabel(updatedLabel);
    }
    m_targetLabel = updatedLabel;
    m_targetLocation = XCAFDoc_ShapeTool::GetLocation(m_targetLabel);
    document->SetModified(m_targetLabel);
    if (openedCommand) document->CommitCommand();

    OCCView* view = m_view.data();
    if (view) {
        view->Context()->SetLocation(m_targetObject, TopLoc_Location(m_parentTransform));
        view->syncShapeSource(m_targetObject);
        view->updateManipulator();
    }
    return true;
}

void WidgetTransform::saveMouseState()
{
    OCCView* view = m_view.data();
    if (!view || m_hasSavedMouseState) return;
    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters = view->getSelectionFilters();
    m_hasSavedMouseState = true;
}

void WidgetTransform::restoreMouseState()
{
    if (!m_hasSavedMouseState) return;
    OCCView* view = m_view.data();
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
    OCCView* view = m_view.data();
    if (!view) {
        m_hasSavedMouseState = false;
        clearTarget();
        m_view.clear();
        return;
    }

    disconnect(view, nullptr, this, nullptr);
    restoreMouseState();
    view->detachManipulator();
    view->removeManipulatorObserver(this);
    clearTarget();
    m_view.clear();
}

void WidgetTransform::restorePreview()
{
    if (m_targetObject.IsNull() || !m_hasOriginalTransform) return;

    if (m_view) {
        applyWorkingTransform(m_originalTransform, TransformSource::Restore);
    } else {
        m_targetObject->SetLocalTransformation(m_originalTransform);
        m_workingTransform = m_originalTransform;
    }
}

void WidgetTransform::clearTarget()
{
    m_targetObject.Nullify();
    m_targetShape.Nullify();
    m_targetLabel.Nullify();
    m_targetLocation = TopLoc_Location();
    m_hasOriginalTransform = false;
    ui->labelObjectName->setText(tr("None"));
    ui->labelStatus->setText(tr("No object selected."));
    ui->pushButtonReset->setEnabled(false);
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
    gp_Trsf worldTransform = trsf;
    worldTransform.Multiply(m_targetLocation.Transformation());
    applyWorkingTransform(worldTransform, TransformSource::Manipulator);
}

void WidgetTransform::onActiveViewChanged(OCCView* view)
{
    if (!isVisible() || (!m_view.isNull() && view == m_view.data())) return;
    close();
}

void WidgetTransform::onShapeObjectsChanged()
{
    if (!m_view || m_targetObject.IsNull()) return;

    const auto& objects = m_view->getShapeObjects();
    if (std::find(objects.cbegin(), objects.cend(), m_targetObject) != objects.cend()) return;

    m_hasOriginalTransform = false;
    m_targetObject.Nullify();
    close();
}

void WidgetTransform::onCancelRequested()
{
    close();
}
