#include "widget_busbar.h"
#include "ui_widget_busbar.h"

#include "OCCView.h"
#include "SelectedEntity.h"
#include "SheetMetalConnectorBuilder.h"
#include "SheetMetalFaceGeometry.h"
#include "SheetMetalRoutePlanner.h"
#include "ViewManager.h"
#include "ViewerWidget.h"

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Quantity_Color.hxx>
#include <StdSelect_ViewerSelector3d.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Comment.hxx>
#include <TDataStd_Name.hxx>
#include <TDF_Tool.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFPrs_AISObject.hxx>

#include <QCloseEvent>
#include <QListWidgetItem>
#include <QMessageBox>

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

WidgetBusbar::WidgetBusbar(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::WidgetBusbar)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    connect(ui->pushButtonPickStart, &QPushButton::clicked,
            this, &WidgetBusbar::onPickStartClicked);
    connect(ui->pushButtonPickStartPoint, &QPushButton::clicked,
            this, &WidgetBusbar::onPickStartPointClicked);
    connect(ui->pushButtonPickEnd, &QPushButton::clicked,
            this, &WidgetBusbar::onPickEndClicked);
    connect(ui->pushButtonPickEndPoint, &QPushButton::clicked,
            this, &WidgetBusbar::onPickEndPointClicked);
    connect(ui->pushButtonAutoCalc, &QPushButton::clicked,
            this, &WidgetBusbar::onAutoCalcClicked);
    connect(ui->pushButtonAddWaypoint, &QPushButton::clicked,
            this, &WidgetBusbar::onAddWaypointClicked);
    connect(ui->pushButtonDeleteWaypoint, &QPushButton::clicked,
            this, &WidgetBusbar::onDeleteWaypointClicked);
    connect(ui->pushButtonApply, &QPushButton::clicked,
            this, &WidgetBusbar::onApplyClicked);
    connect(ui->pushButtonClear, &QPushButton::clicked,
            this, &WidgetBusbar::onClearClicked);
    connect(ui->pushButtonClose, &QPushButton::clicked,
            this, &WidgetBusbar::onCloseClicked);
    connect(ui->listWidgetWaypoints, &QListWidget::currentItemChanged,
            this, &WidgetBusbar::onWaypointSelected);

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

WidgetBusbar::~WidgetBusbar()
{
    stopFacePicking();
    clearPreview();
    delete ui;
}

void WidgetBusbar::show()
{
    QWidget::show();
    refreshWaypointList();
}

void WidgetBusbar::closeEvent(QCloseEvent* event)
{
    stopFacePicking();
    clearPreview();
    QWidget::closeEvent(event);
}

void WidgetBusbar::onPickStartClicked()
{
    startPicking(PickingState::StartFace);
}

void WidgetBusbar::onPickStartPointClicked()
{
    if (!m_hasStart) {
        updateStatus(tr("Select the start connection face first."));
        return;
    }
    startPicking(PickingState::StartPoint);
}

void WidgetBusbar::onPickEndClicked()
{
    startPicking(PickingState::EndFace);
}

void WidgetBusbar::onPickEndPointClicked()
{
    if (!m_hasEnd) {
        updateStatus(tr("Select the end connection face first."));
        return;
    }
    startPicking(PickingState::EndPoint);
}

void WidgetBusbar::startPicking(PickingState state)
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
            this, &WidgetBusbar::onObjectSelected, Qt::UniqueConnection);

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

void WidgetBusbar::stopFacePicking()
{
    if (m_pickingState == PickingState::Idle) {
        return;
    }

    if (m_pickView) {
        disconnect(m_pickView, &OCCView::signalSpaceSelected,
                   this, &WidgetBusbar::onObjectSelected);
        m_pickView->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
        for (const auto& filter : m_savedFilters) {
            m_pickView->updateSelectionFilter(filter.first, filter.second);
        }
    }

    m_pickView.clear();
    m_pickingState = PickingState::Idle;
}

void WidgetBusbar::onObjectSelected(const TopoDS_Shape& shape)
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

bool WidgetBusbar::updateConnectionFromSelection(SheetMetalConnectionEnd& connection,
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

bool WidgetBusbar::updatePointFromSelection(
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

void WidgetBusbar::onAutoCalcClicked()
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

void WidgetBusbar::onAddWaypointClicked()
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

void WidgetBusbar::onDeleteWaypointClicked()
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

void WidgetBusbar::refreshWaypointList()
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

void WidgetBusbar::onWaypointSelected(QListWidgetItem* current,
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

void WidgetBusbar::updateNodePropertyPanel()
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

void WidgetBusbar::saveCurrentNodeProps()
{
    if (m_selectedNodeIndex <= 0
        || m_selectedNodeIndex >= static_cast<int>(m_data.routePoints.size()) - 1) {
        return;
    }
    m_data.routePoints[static_cast<std::size_t>(m_selectedNodeIndex)].bend.radius =
        ui->spinBoxBendRadius->value();
}

void WidgetBusbar::updateDataFromUi()
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

void WidgetBusbar::updatePreview()
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

void WidgetBusbar::onApplyClicked()
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
        const QByteArray metadata = connectorMetadata().toUtf8();
        TDataStd_Comment::Set(label, TCollection_ExtendedString(metadata.constData()));
        viewer->updateTree();
    }

    m_appliedShape = buildResult.solid;
    clearPreview();
    updateStatus(tr("Sheet-metal connector generated successfully."));
}

QString WidgetBusbar::connectorMetadata() const
{
    const auto labelEntry = [](const TDF_Label& label) {
        if (label.IsNull()) {
            return QString();
        }
        TCollection_AsciiString entry;
        TDF_Tool::Entry(label, entry);
        return QString::fromLatin1(entry.ToCString());
    };
    const auto connectionText = [&labelEntry](const SheetMetalConnectionEnd& connection) {
        return QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10")
            .arg(connection.position.X(), 0, 'g', 12)
            .arg(connection.position.Y(), 0, 'g', 12)
            .arg(connection.position.Z(), 0, 'g', 12)
            .arg(connection.normal.X(), 0, 'g', 12)
            .arg(connection.normal.Y(), 0, 'g', 12)
            .arg(connection.normal.Z(), 0, 'g', 12)
            .arg(static_cast<int>(connection.jointType))
            .arg(connection.boltNut.specIndex)
            .arg(connection.weld.typeIndex)
            .arg(labelEntry(connection.targetLabel));
    };

    QStringList routePoints;
    routePoints.reserve(static_cast<int>(m_data.routePoints.size()));
    for (const auto& point : m_data.routePoints) {
        routePoints.push_back(QStringLiteral("%1,%2,%3,%4")
            .arg(point.position.X(), 0, 'g', 12)
            .arg(point.position.Y(), 0, 'g', 12)
            .arg(point.position.Z(), 0, 'g', 12)
            .arg(point.bend.radius, 0, 'g', 12));
    }

    return QStringLiteral(
        "version=1;width=%1;thickness=%2;clearance=%3;start=%4;end=%5;route=%6")
        .arg(m_data.width, 0, 'g', 12)
        .arg(m_data.thickness, 0, 'g', 12)
        .arg(m_data.clearance, 0, 'g', 12)
        .arg(connectionText(m_data.start))
        .arg(connectionText(m_data.end))
        .arg(routePoints.join(QLatin1Char('|')));
}

void WidgetBusbar::onClearClicked()
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

void WidgetBusbar::onCloseClicked()
{
    close();
}

void WidgetBusbar::clearPreview()
{
    auto* view = m_previewView.data();
    if (view && !view->Context().IsNull() && !m_previewShape.IsNull()) {
        view->Context()->Remove(m_previewShape, Standard_False);
        view->Context()->UpdateCurrentViewer();
    }
    m_previewShape.Nullify();
    m_previewView.clear();
}

void WidgetBusbar::updateStatus(const QString& message)
{
    ui->labelStatus->setText(message);
}
