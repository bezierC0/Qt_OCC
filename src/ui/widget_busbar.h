#pragma once

#include "SheetMetalCollisionChecker.h"
#include "SheetMetalConnectorModel.h"

#include <QPointer>
#include <QWidget>
#include <map>
#include <vector>

// OCC
#include <AIS_Shape.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>

class QCloseEvent;
class QListWidgetItem;
class OCCView;

namespace Ui
{
class WidgetBusbar;
}

// ============================================================
// WidgetBusbar
// ============================================================

/**
 * @brief Panel for connecting two faces with routed sheet metal.
 *
 * Workflow:
 *  1. Pick start and end faces interactively in the 3-D view.
 *  2. Click "Auto Calculate Route" to generate an orthogonal path.
 *  3. Add / delete / edit bend waypoints.
 *  4. Set sheet-metal and endpoint joint parameters.
 *  5. Validate collisions and apply the generated solid.
 */
class WidgetBusbar : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetBusbar(QWidget *parent = nullptr);
    ~WidgetBusbar() override;

    void show();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onPickStartClicked();
    void onPickStartPointClicked();
    void onPickEndClicked();
    void onPickEndPointClicked();
    void onObjectSelected(const TopoDS_Shape& shape);

    void onAutoCalcClicked();
    void onAddWaypointClicked();
    void onDeleteWaypointClicked();

    void onWaypointSelected(QListWidgetItem *current, QListWidgetItem *previous);

    void onApplyClicked();
    void onClearClicked();
    void onCloseClicked();

private:
    enum class PickingState
    {
        Idle,
        StartFace,
        StartPoint,
        EndFace,
        EndPoint
    };

    void startPicking(PickingState state);
    void stopFacePicking();
    bool updateConnectionFromSelection(SheetMetalConnectionEnd& connection,
                                       const TopoDS_Face& face,
                                       const Handle(AIS_InteractiveObject)& parentObject);
    bool updatePointFromSelection(SheetMetalConnectionEnd& connection,
                                  const TopoDS_Face& face,
                                  const Handle(AIS_InteractiveObject)& parentObject);
    void updateDataFromUi();
    void updatePreview();
    QString connectorMetadata() const;
    void refreshWaypointList();
    void updateNodePropertyPanel();
    void saveCurrentNodeProps();
    void updateStatus(const QString &msg);
    void clearPreview();

private:
    Ui::WidgetBusbar *ui{};

    SheetMetalConnectorData m_data;
    bool m_hasStart{false};
    bool m_hasEnd{false};
    int m_selectedNodeIndex{-1};

    PickingState m_pickingState{PickingState::Idle};
    QPointer<OCCView> m_pickView;
    QPointer<OCCView> m_modelView;
    QPointer<OCCView> m_previewView;
    int m_savedMouseMode{0};
    std::map<TopAbs_ShapeEnum, bool> m_savedFilters;

    SheetMetalCollisionResult m_collisionResult;
    Handle(AIS_Shape) m_previewShape;
    TopoDS_Shape m_appliedShape;
};
