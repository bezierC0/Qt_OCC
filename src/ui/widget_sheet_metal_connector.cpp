#include "widget_sheet_metal_connector.h"
#include "ui_widget_sheet_metal_connector.h"

#include "OCCView.h"
#include "SelectedEntity.h"
#include "SheetMetalConnectorBuilder.h"
#include "SheetMetalFaceGeometry.h"
#include "SheetMetalConnectorMetadata.h"
#include "SheetMetalRoutePlanner.h"
#include "ViewManager.h"
#include "ViewerWidget.h"

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Quantity_Color.hxx>
#include <StdSelect_ViewerSelector3d.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Comment.hxx>
#include <TDataStd_Name.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFPrs_AISObject.hxx>

#include <QCloseEvent>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QSignalBlocker>

#include <cmath>
#include <utility>

namespace
{
TopoDS_Shape shapeFromObject(const Handle(AIS_InteractiveObject)& object)
{
    if (object.IsNull()) {
        return {};
    }

    TopoDS_Shape shape;
    const auto aisShape = Handle(AIS_Shape)::DownCast(object);
    if (!aisShape.IsNull()) {
        shape = aisShape->Shape();
    } else {
        const auto xcafObject = Handle(XCAFPrs_AISObject)::DownCast(object);
        if (!xcafObject.IsNull()) {
            XCAFDoc_ShapeTool::GetShape(xcafObject->GetLabel(), shape);
        }
    }

    if (!shape.IsNull() && object->LocalTransformation().Form() != gp_Identity) {
        shape.Location(TopLoc_Location(object->LocalTransformation()) * shape.Location());
    }
    return shape;
}

QString pointText(const gp_Pnt& point)
{
    return QStringLiteral("(%1, %2, %3)")
        .arg(point.X(), 0, 'f', 1)
        .arg(point.Y(), 0, 'f', 1)
        .arg(point.Z(), 0, 'f', 1);
}

QString directionText(const gp_Dir& direction)
{
    return QStringLiteral("(%1, %2, %3)")
        .arg(direction.X(), 0, 'f', 3)
        .arg(direction.Y(), 0, 'f', 3)
        .arg(direction.Z(), 0, 'f', 3);
}
}

WidgetSheetMetalConnector::WidgetSheetMetalConnector(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::WidgetSheetMetalConnector)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    connect(ui->pushButtonPickStart, &QPushButton::clicked,
            this, &WidgetSheetMetalConnector::onPickStartClicked);
    connect(ui->pushButtonPickStartPoint, &QPushButton::clicked,
            this, &WidgetSheetMetalConnector::onPickStartPointClicked);
    connect(ui->pushButtonPickEnd, &QPushButton::clicked,
            this, &WidgetSheetMetalConnector::onPickEndClicked);
    connect(ui->pushButtonPickEndPoint, &QPushButton::clicked,
            this, &WidgetSheetMetalConnector::onPickEndPointClicked);
    connect(ui->pushButtonAutoCalc, &QPushButton::clicked,
            this, &WidgetSheetMetalConnector::onAutoCalcClicked);
    connect(ui->pushButtonAddWaypoint, &QPushButton::clicked,
            this, &WidgetSheetMetalConnector::onAddWaypointClicked);
    connect(ui->pushButtonDeleteWaypoint, &QPushButton::clicked,
            this, &WidgetSheetMetalConnector::onDeleteWaypointClicked);
    connect(ui->pushButtonApply, &QPushButton::clicked,
            this, &WidgetSheetMetalConnector::onApplyClicked);
    connect(ui->pushButtonClear, &QPushButton::clicked,
            this, &WidgetSheetMetalConnector::onClearClicked);
    connect(ui->pushButtonClose, &QPushButton::clicked,
            this, &WidgetSheetMetalConnector::onCloseClicked);
    connect(ui->listWidgetWaypoints, &QListWidget::currentItemChanged,
            this, &WidgetSheetMetalConnector::onWaypointSelected);

    const auto refreshPreview = [this]() {
        if (m_data.routePoints.size() >= 2) {
            updatePreview();
        }
    };
    connect(ui->spinBoxWidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, refreshPreview);
    connect(ui->spinBoxThickness, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, refreshPreview);
    connect(ui->spinBoxClearance, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, refreshPreview);
    connect(ui->comboBoxStartJoint, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, refreshPreview);
    connect(ui->comboBoxEndJoint, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, refreshPreview);
    connect(ui->comboBoxStartBoltSpec, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, refreshPreview);
    connect(ui->comboBoxEndBoltSpec, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, refreshPreview);
    connect(ui->comboBoxSectionDir, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, refreshPreview);
    connect(ui->spinBoxBendRadius, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) {
                if (m_selectedNodeIndex > 0
                    && m_selectedNodeIndex < static_cast<int>(m_data.routePoints.size()) - 1) {
                    saveCurrentNodeProps();
                    updatePreview();
                    refreshWaypointList();
                }
            });

    const auto updateStartJointControls = [this](int index) {
        const bool isWeld = index == 0;
        ui->labelStartWeldType->setEnabled(isWeld);
        ui->comboBoxStartWeldType->setEnabled(isWeld);
        ui->labelStartBoltSpec->setEnabled(!isWeld);
        ui->comboBoxStartBoltSpec->setEnabled(!isWeld);
    };
    const auto updateEndJointControls = [this](int index) {
        const bool isWeld = index == 0;
        ui->labelEndWeldType->setEnabled(isWeld);
        ui->comboBoxEndWeldType->setEnabled(isWeld);
        ui->labelEndBoltSpec->setEnabled(!isWeld);
        ui->comboBoxEndBoltSpec->setEnabled(!isWeld);
    };
    connect(ui->comboBoxStartJoint, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, updateStartJointControls);
    connect(ui->comboBoxEndJoint, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, updateEndJointControls);
    updateStartJointControls(ui->comboBoxStartJoint->currentIndex());
    updateEndJointControls(ui->comboBoxEndJoint->currentIndex());

    updateStatus(tr("Ready"));
}

WidgetSheetMetalConnector::~WidgetSheetMetalConnector()
{
    stopFacePicking();
    clearPreview();
    delete ui;
}

void WidgetSheetMetalConnector::show()
{
    loadSelectedConnector();
    QWidget::show();
    refreshWaypointList();
}

void WidgetSheetMetalConnector::closeEvent(QCloseEvent* event)
{
    stopFacePicking();
    clearPreview();
    QWidget::closeEvent(event);
}

void WidgetSheetMetalConnector::onPickStartClicked()
{
    startPicking(PickingState::StartFace);
}

void WidgetSheetMetalConnector::onPickStartPointClicked()
{
    if (!m_hasStart || m_data.start.face.IsNull()) {
        updateStatus(tr("Select the start connection face first."));
        return;
    }
    startPicking(PickingState::StartPoint);
}

void WidgetSheetMetalConnector::onPickEndClicked()
{
    startPicking(PickingState::EndFace);
}

void WidgetSheetMetalConnector::onPickEndPointClicked()
{
    if (!m_hasEnd || m_data.end.face.IsNull()) {
        updateStatus(tr("Select the end connection face first."));
        return;
    }
    startPicking(PickingState::EndPoint);
}

void WidgetSheetMetalConnector::startPicking(PickingState state)
{
    stopFacePicking();

    auto* view = ViewManager::getInstance().getActiveView();
    if (!view) {
        updateStatus(tr("No active view available."));
        return;
    }
    if (m_modelView && view != m_modelView) {
        updateStatus(tr("Select both connection faces in the same view."));
        return;
    }

    m_pickView = view;
    m_pickingState = state;
    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters = view->getSelectionFilters();

    view->clearSelectedObjects();
    for (const auto& filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, false);
    }
    view->updateSelectionFilter(TopAbs_FACE, true);
    view->setMouseMode(View::MouseMode::SELECTION);

    connect(view, &OCCView::signalSpaceSelected,
            this, &WidgetSheetMetalConnector::onObjectSelected, Qt::UniqueConnection);

    if (state == PickingState::StartFace) {
        updateStatus(tr("Select the start connection face."));
    } else if (state == PickingState::StartPoint) {
        updateStatus(tr("Select a point on the start connection face."));
    } else if (state == PickingState::EndFace) {
        updateStatus(tr("Select the end connection face."));
    } else {
        updateStatus(tr("Select a point on the end connection face."));
    }
}

void WidgetSheetMetalConnector::stopFacePicking()
{
    if (m_pickingState == PickingState::Idle) {
        return;
    }

    if (m_pickView) {
        disconnect(m_pickView, &OCCView::signalSpaceSelected,
                   this, &WidgetSheetMetalConnector::onObjectSelected);
        m_pickView->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
        for (const auto& filter : m_savedFilters) {
            m_pickView->updateSelectionFilter(filter.first, filter.second);
        }
    }

    m_pickView.clear();
    m_pickingState = PickingState::Idle;
}

void WidgetSheetMetalConnector::onObjectSelected(const TopoDS_Shape& shape)
{
    if (m_pickingState == PickingState::Idle
        || shape.IsNull()
        || shape.ShapeType() != TopAbs_FACE) {
        return;
    }

    const bool isStart = m_pickingState == PickingState::StartFace
        || m_pickingState == PickingState::StartPoint;
    const bool isPoint = m_pickingState == PickingState::StartPoint
        || m_pickingState == PickingState::EndPoint;
    SheetMetalConnectionEnd& connection = isStart
        ? m_data.start
        : m_data.end;
    const PickingState completedState = m_pickingState;

    const auto& selectedObjects = m_pickView->getSelectedObjects();
    if (selectedObjects.empty() || !selectedObjects.back()) {
        updateStatus(tr("Failed to evaluate the selected face."));
        return;
    }
    const auto& entity = selectedObjects.back();
    const Handle(AIS_InteractiveObject) parentObject = entity->GetParentInteractiveObject();
    const bool updated = isPoint
        ? updatePointFromSelection(connection, TopoDS::Face(shape), parentObject)
        : updateConnectionFromSelection(connection, TopoDS::Face(shape), parentObject);
    if (!updated) {
        updateStatus(isPoint
            ? tr("Select a point on the previously selected face.")
            : tr("Failed to evaluate the selected face."));
        return;
    }

    if (completedState == PickingState::StartFace) {
        const bool sameView = !m_modelView || m_modelView == m_pickView;
        m_modelView = m_pickView;
        if (!sameView) {
            m_data.end = SheetMetalConnectionEnd();
            m_hasEnd = false;
            ui->labelEndPosValue->setText(QStringLiteral("-"));
            ui->labelEndNormalValue->setText(QStringLiteral("-"));
        }
        m_hasStart = true;
        ui->labelStartPosValue->setText(pointText(connection.position));
        ui->labelStartNormalValue->setText(directionText(connection.normal));
    } else if (completedState == PickingState::EndFace) {
        m_hasEnd = true;
        ui->labelEndPosValue->setText(pointText(connection.position));
        ui->labelEndNormalValue->setText(directionText(connection.normal));
    } else if (completedState == PickingState::StartPoint) {
        ui->labelStartPosValue->setText(pointText(connection.position));
    } else {
        ui->labelEndPosValue->setText(pointText(connection.position));
    }

    stopFacePicking();
    m_data.routePoints.clear();
    clearPreview();
    refreshWaypointList();
    updateStatus(isPoint
        ? tr("Connection point selected.")
        : tr("Connection face selected. A default point was placed on the face."));
}

bool WidgetSheetMetalConnector::updateConnectionFromSelection(SheetMetalConnectionEnd& connection,
                                                  const TopoDS_Face& face,
                                                  const Handle(AIS_InteractiveObject)& parentObject)
{
    if (!m_pickView || face.IsNull() || parentObject.IsNull()) {
        return false;
    }

    TopoDS_Face worldFace = face;
    if (parentObject->LocalTransformation().Form() != gp_Identity) {
        worldFace.Location(TopLoc_Location(parentObject->LocalTransformation())
                           * worldFace.Location());
    }

    gp_Pnt centerPoint;
    gp_Dir faceNormal;
    if (!SheetMetalFaceGeometry::centerPointAndNormal(
            worldFace, centerPoint, faceNormal)) {
        return false;
    }

    connection.position = centerPoint;
    connection.normal = faceNormal;
    connection.face = worldFace;
    connection.targetShape = shapeFromObject(parentObject);
    connection.targetLabel = m_pickView->getSelectedObjects().back()->GetLabel();
    return !connection.targetShape.IsNull();
}

bool WidgetSheetMetalConnector::updatePointFromSelection(
    SheetMetalConnectionEnd& connection,
    const TopoDS_Face& face,
    const Handle(AIS_InteractiveObject)& parentObject)
{
    if (!m_pickView || face.IsNull() || parentObject.IsNull()
        || connection.face.IsNull()) {
        return false;
    }

    TopoDS_Face worldFace = face;
    if (parentObject->LocalTransformation().Form() != gp_Identity) {
        worldFace.Location(TopLoc_Location(parentObject->LocalTransformation())
                           * worldFace.Location());
    }
    if (!worldFace.IsSame(connection.face)) {
        return false;
    }

    const Handle(StdSelect_ViewerSelector3d)& selector =
        m_pickView->Context()->MainSelector();
    if (selector.IsNull() || selector->NbPicked() < 1) {
        return false;
    }

    gp_Pnt selectedPoint;
    if (!SheetMetalFaceGeometry::projectPoint(
            connection.face, selector->PickedPoint(1), selectedPoint)) {
        return false;
    }
    connection.position = selectedPoint;
    return true;
}

void WidgetSheetMetalConnector::onAutoCalcClicked()
{
    if (!m_hasStart || !m_hasEnd) {
        updateStatus(tr("Select the start and end connection faces first."));
        return;
    }

    updateDataFromUi();
    QString errorMessage;
    if (!SheetMetalRoutePlanner::computeOrthogonalRoute(
            m_data, ui->spinBoxDefaultBendRadius->value(), &errorMessage)) {
        updateStatus(tr("Route calculation failed: %1").arg(errorMessage));
        return;
    }

    m_selectedNodeIndex = -1;
    refreshWaypointList();
    updatePreview();
}

void WidgetSheetMetalConnector::onAddWaypointClicked()
{
    if (m_data.routePoints.size() < 2) {
        updateStatus(tr("Calculate a route before adding waypoints."));
        return;
    }

    const auto& previous = m_data.routePoints[m_data.routePoints.size() - 2];
    const auto& last = m_data.routePoints.back();
    SheetMetalRoutePoint waypoint;
    waypoint.position = gp_Pnt(
        (previous.position.X() + last.position.X()) * 0.5,
        (previous.position.Y() + last.position.Y()) * 0.5,
        (previous.position.Z() + last.position.Z()) * 0.5);
    waypoint.bend.radius = ui->spinBoxDefaultBendRadius->value();
    m_data.routePoints.insert(m_data.routePoints.end() - 1, waypoint);
    refreshWaypointList();
    updatePreview();
}

void WidgetSheetMetalConnector::onDeleteWaypointClicked()
{
    const int row = ui->listWidgetWaypoints->currentRow();
    if (row < 0 || row >= static_cast<int>(m_data.routePoints.size())) {
        return;
    }
    if (m_data.routePoints[static_cast<std::size_t>(row)].isEndpoint) {
        updateStatus(tr("Start and end points cannot be deleted."));
        return;
    }

    m_data.routePoints.erase(m_data.routePoints.begin() + row);
    m_selectedNodeIndex = -1;
    refreshWaypointList();
    updatePreview();
}

void WidgetSheetMetalConnector::refreshWaypointList()
{
    ui->listWidgetWaypoints->blockSignals(true);
    ui->listWidgetWaypoints->clear();
    for (std::size_t index = 0; index < m_data.routePoints.size(); ++index) {
        const auto& point = m_data.routePoints[index];
        QString label;
        if (index == 0) {
            label = tr("P0 Start");
        } else if (index + 1 == m_data.routePoints.size()) {
            label = tr("P%1 End").arg(index);
        } else {
            label = tr("P%1 Bend R=%2 mm")
                .arg(index)
                .arg(point.bend.radius, 0, 'f', 1);
        }
        ui->listWidgetWaypoints->addItem(label + QStringLiteral("  ") + pointText(point.position));
    }
    if (m_selectedNodeIndex >= 0
        && m_selectedNodeIndex < ui->listWidgetWaypoints->count()) {
        ui->listWidgetWaypoints->setCurrentRow(m_selectedNodeIndex);
    }
    ui->listWidgetWaypoints->blockSignals(false);
    updateNodePropertyPanel();
}

void WidgetSheetMetalConnector::onWaypointSelected(QListWidgetItem* current,
                                       QListWidgetItem* previous)
{
    Q_UNUSED(previous)
    if (m_selectedNodeIndex >= 0) {
        saveCurrentNodeProps();
    }
    m_selectedNodeIndex = current
        ? ui->listWidgetWaypoints->row(current)
        : -1;
    updateNodePropertyPanel();
}

void WidgetSheetMetalConnector::updateNodePropertyPanel()
{
    const bool editable = m_selectedNodeIndex > 0
        && m_selectedNodeIndex < static_cast<int>(m_data.routePoints.size()) - 1;
    ui->groupBoxNodeProp->setEnabled(editable);
    if (!editable) {
        return;
    }

    ui->spinBoxBendRadius->setValue(
        m_data.routePoints[static_cast<std::size_t>(m_selectedNodeIndex)].bend.radius);
}

void WidgetSheetMetalConnector::saveCurrentNodeProps()
{
    if (m_selectedNodeIndex <= 0
        || m_selectedNodeIndex >= static_cast<int>(m_data.routePoints.size()) - 1) {
        return;
    }
    m_data.routePoints[static_cast<std::size_t>(m_selectedNodeIndex)].bend.radius =
        ui->spinBoxBendRadius->value();
}

void WidgetSheetMetalConnector::updateDataFromUi()
{
    saveCurrentNodeProps();
    m_data.width = ui->spinBoxWidth->value();
    m_data.thickness = ui->spinBoxThickness->value();
    m_data.clearance = ui->spinBoxClearance->value();
    m_data.start.jointType = ui->comboBoxStartJoint->currentIndex() == 0
        ? SheetMetalJointType::Weld
        : SheetMetalJointType::BoltNut;
    m_data.end.jointType = ui->comboBoxEndJoint->currentIndex() == 0
        ? SheetMetalJointType::Weld
        : SheetMetalJointType::BoltNut;
    m_data.start.boltNut.specIndex = ui->comboBoxStartBoltSpec->currentIndex();
    m_data.end.boltNut.specIndex = ui->comboBoxEndBoltSpec->currentIndex();
    m_data.start.weld.typeIndex = ui->comboBoxStartWeldType->currentIndex();
    m_data.end.weld.typeIndex = ui->comboBoxEndWeldType->currentIndex();

    const int directionIndex = ui->comboBoxSectionDir->currentIndex();
    if (directionIndex == 1) {
        m_data.upDirection = gp_Dir(1.0, 0.0, 0.0);
    } else if (directionIndex == 2) {
        m_data.upDirection = gp_Dir(0.0, 1.0, 0.0);
    } else {
        m_data.upDirection = gp_Dir(0.0, 0.0, 1.0);
    }
}

void WidgetSheetMetalConnector::updateUiFromData()
{
    const QSignalBlocker widthBlocker(ui->spinBoxWidth);
    const QSignalBlocker thicknessBlocker(ui->spinBoxThickness);
    const QSignalBlocker clearanceBlocker(ui->spinBoxClearance);
    const QSignalBlocker directionBlocker(ui->comboBoxSectionDir);
    const QSignalBlocker startJointBlocker(ui->comboBoxStartJoint);
    const QSignalBlocker startBoltBlocker(ui->comboBoxStartBoltSpec);
    const QSignalBlocker startWeldBlocker(ui->comboBoxStartWeldType);
    const QSignalBlocker endJointBlocker(ui->comboBoxEndJoint);
    const QSignalBlocker endBoltBlocker(ui->comboBoxEndBoltSpec);
    const QSignalBlocker endWeldBlocker(ui->comboBoxEndWeldType);

    ui->spinBoxWidth->setValue(m_data.width);
    ui->spinBoxThickness->setValue(m_data.thickness);
    ui->spinBoxClearance->setValue(m_data.clearance);
    ui->comboBoxSectionDir->setCurrentIndex(
        std::abs(m_data.upDirection.X()) > 0.999 ? 1
        : std::abs(m_data.upDirection.Y()) > 0.999 ? 2
        : 0);
    ui->comboBoxStartJoint->setCurrentIndex(
        m_data.start.jointType == SheetMetalJointType::Weld ? 0 : 1);
    ui->comboBoxStartBoltSpec->setCurrentIndex(m_data.start.boltNut.specIndex);
    ui->comboBoxStartWeldType->setCurrentIndex(m_data.start.weld.typeIndex);
    ui->comboBoxEndJoint->setCurrentIndex(
        m_data.end.jointType == SheetMetalJointType::Weld ? 0 : 1);
    ui->comboBoxEndBoltSpec->setCurrentIndex(m_data.end.boltNut.specIndex);
    ui->comboBoxEndWeldType->setCurrentIndex(m_data.end.weld.typeIndex);

    const bool startIsWeld = m_data.start.jointType == SheetMetalJointType::Weld;
    ui->labelStartWeldType->setEnabled(startIsWeld);
    ui->comboBoxStartWeldType->setEnabled(startIsWeld);
    ui->labelStartBoltSpec->setEnabled(!startIsWeld);
    ui->comboBoxStartBoltSpec->setEnabled(!startIsWeld);
    const bool endIsWeld = m_data.end.jointType == SheetMetalJointType::Weld;
    ui->labelEndWeldType->setEnabled(endIsWeld);
    ui->comboBoxEndWeldType->setEnabled(endIsWeld);
    ui->labelEndBoltSpec->setEnabled(!endIsWeld);
    ui->comboBoxEndBoltSpec->setEnabled(!endIsWeld);

    ui->labelStartPosValue->setText(pointText(m_data.start.position));
    ui->labelStartNormalValue->setText(directionText(m_data.start.normal));
    ui->labelEndPosValue->setText(pointText(m_data.end.position));
    ui->labelEndNormalValue->setText(directionText(m_data.end.normal));
}

bool WidgetSheetMetalConnector::loadSelectedConnector()
{
    auto* viewer = qobject_cast<ViewerWidget*>(parentWidget());
    if (!viewer) {
        return false;
    }

    QString metadata;
    TDF_Label connectorLabel;
    TopoDS_Shape connectorShape;
    if (!viewer->selectedSheetMetalConnector(
            metadata, connectorLabel, connectorShape)) {
        return false;
    }

    SheetMetalConnectorMetadataResult parsed;
    QString errorMessage;
    if (!SheetMetalConnectorMetadata::deserialize(
            metadata, parsed, &errorMessage)) {
        updateStatus(tr("Failed to load connector parameters: %1").arg(errorMessage));
        return false;
    }

    if (!viewer->resolveShapeLabel(parsed.startTargetEntry,
                                   parsed.data.start.targetLabel,
                                   parsed.data.start.targetShape)
        || !viewer->resolveShapeLabel(parsed.endTargetEntry,
                                      parsed.data.end.targetLabel,
                                      parsed.data.end.targetShape)) {
        updateStatus(tr("Failed to restore the connector target shapes."));
        return false;
    }

    clearPreview();
    m_data = std::move(parsed.data);
    m_hasStart = true;
    m_hasEnd = true;
    m_modelView = ViewManager::getInstance().getActiveView();
    m_appliedShape = connectorShape;
    m_selectedNodeIndex = -1;
    updateUiFromData();
    refreshWaypointList();
    updateStatus(tr("Connector parameters loaded. Reselect a face before changing its point."));
    return true;
}

void WidgetSheetMetalConnector::updatePreview()
{
    updateDataFromUi();
    clearPreview();

    const auto buildResult = SheetMetalConnectorBuilder::build(m_data);
    if (!buildResult.isValid()) {
        updateStatus(tr("Preview failed: %1").arg(buildResult.errorMessage));
        return;
    }

    auto* view = m_modelView.data();
    if (!view || view->Context().IsNull()) {
        updateStatus(tr("No active view available."));
        return;
    }

    m_collisionResult = SheetMetalCollisionChecker::check(
        buildResult.solid, m_data, view->getShapeObjects(), m_appliedShape);
    m_previewShape = new AIS_Shape(buildResult.solid);
    m_previewView = view;
    m_previewShape->SetColor(m_collisionResult.isAcceptable()
        ? Quantity_NOC_CYAN1
        : Quantity_NOC_RED);
    m_previewShape->SetTransparency(0.35);
    m_previewShape->SetZLayer(Graphic3d_ZLayerId_Topmost);
    view->Context()->Display(m_previewShape, Standard_False);
    view->Context()->UpdateCurrentViewer();

    updateStatus(m_collisionResult.isAcceptable()
        ? tr("Route is valid and ready to apply.")
        : tr("Route has a collision or clearance violation."));
}

void WidgetSheetMetalConnector::onApplyClicked()
{
    if (!m_hasStart || !m_hasEnd || m_data.routePoints.size() < 2) {
        updateStatus(tr("Select two faces and calculate a route first."));
        return;
    }

    updateDataFromUi();
    const auto buildResult = SheetMetalConnectorBuilder::build(m_data);
    if (!buildResult.isValid()) {
        updateStatus(tr("Failed to generate sheet metal: %1").arg(buildResult.errorMessage));
        return;
    }

    auto* view = m_modelView.data();
    if (!view) {
        updateStatus(tr("No active view available."));
        return;
    }

    m_collisionResult = SheetMetalCollisionChecker::check(
        buildResult.solid, m_data, view->getShapeObjects(), m_appliedShape);
    if (!m_collisionResult.isAcceptable() && !ui->checkBoxAllowCollision->isChecked()) {
        QMessageBox::warning(
            this,
            tr("Sheet Metal Connector"),
            tr("The connector collides with another shape or violates the minimum clearance."));
        updatePreview();
        return;
    }

    auto* viewer = qobject_cast<ViewerWidget*>(parentWidget());
    if (!viewer) {
        updateStatus(tr("The owning viewer is not available."));
        return;
    }

    if (!m_appliedShape.IsNull()) {
        viewer->removeShape(m_appliedShape);
    }
    const TDF_Label label = viewer->displayShape(buildResult.solid, 0.83, 0.63, 0.16);
    if (!label.IsNull()) {
        TDataStd_Name::Set(label, "Sheet Metal Connector");
        const QByteArray metadata = SheetMetalConnectorMetadata::serialize(m_data).toUtf8();
        TDataStd_Comment::Set(label, TCollection_ExtendedString(metadata.constData()));
        viewer->updateTree();
    }

    m_appliedShape = buildResult.solid;
    clearPreview();
    updateStatus(tr("Sheet-metal connector generated successfully."));
}

void WidgetSheetMetalConnector::onClearClicked()
{
    stopFacePicking();
    clearPreview();

    if (!m_appliedShape.IsNull()) {
        if (auto* viewer = qobject_cast<ViewerWidget*>(parentWidget())) {
            viewer->removeShape(m_appliedShape);
        }
        m_appliedShape.Nullify();
    }

    m_data = SheetMetalConnectorData();
    m_hasStart = false;
    m_hasEnd = false;
    m_modelView.clear();
    m_selectedNodeIndex = -1;
    ui->labelStartPosValue->setText(QStringLiteral("-"));
    ui->labelStartNormalValue->setText(QStringLiteral("-"));
    ui->labelEndPosValue->setText(QStringLiteral("-"));
    ui->labelEndNormalValue->setText(QStringLiteral("-"));
    refreshWaypointList();
    updateStatus(tr("Cleared."));
}

void WidgetSheetMetalConnector::onCloseClicked()
{
    close();
}

void WidgetSheetMetalConnector::clearPreview()
{
    auto* view = m_previewView.data();
    if (view && !view->Context().IsNull() && !m_previewShape.IsNull()) {
        view->Context()->Remove(m_previewShape, Standard_False);
        view->Context()->UpdateCurrentViewer();
    }
    m_previewShape.Nullify();
    m_previewView.clear();
}

void WidgetSheetMetalConnector::updateStatus(const QString& message)
{
    ui->labelStatus->setText(message);
}
