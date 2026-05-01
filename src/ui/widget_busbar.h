#pragma once

#include <QWidget>
#include <vector>

// OCC
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <TopoDS_Shape.hxx>
#include <AIS_Shape.hxx>

class QCloseEvent;
class QListWidgetItem;
class ViewerPickHelper;

namespace Ui
{
class WidgetBusbar;
}

// ============================================================
// Data structures
// ============================================================

/// Connection type at each node
enum class ConnectionType
{
    Bend, ///< Bending
    Bolt, ///< Bolted joint
    Weld  ///< Welded joint
};

/// Bend parameters
struct BendParam
{
    double radius{5.0}; ///< Bend radius [mm]
};

/// Bolt parameters
struct BoltParam
{
    int specIndex{0}; ///< M6=0, M8=1, M10=2, M12=3
};

/// Weld parameters
struct WeldParam
{
    int typeIndex{0}; ///< Butt=0, Lap=1
};

/// A single node on the busbar route (start / end / waypoint)
struct BusbarNode
{
    gp_Pnt         position;
    gp_Dir         normal{0, 0, 1};
    ConnectionType type{ConnectionType::Bend};
    BendParam      bend;
    BoltParam      bolt;
    WeldParam      weld;
    bool           isEndpoint{false}; ///< true = start or end; cannot be deleted
};

// ============================================================
// WidgetBusbar
// ============================================================

/**
 * @brief Panel for 3-D busbar routing.
 *
 * Workflow:
 *  1. Pick start and end points interactively in the 3-D view.
 *  2. Click "Auto Calculate Route" to generate an orthogonal path.
 *  3. Add / delete / edit waypoint connection properties.
 *  4. Set busbar cross-section parameters (width, thickness, section direction).
 *  5. Click "Apply" to generate a solid via rectangular-section sweep.
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
    // Pick buttons
    void onPickStartClicked();
    void onPickEndClicked();
    void onStartPicked(double x, double y, double z);
    void onEndPicked(double x, double y, double z);

    // Route operations
    void onAutoCalcClicked();
    void onAddWaypointClicked();
    void onDeleteWaypointClicked();

    // Node property panel
    void onWaypointSelected(QListWidgetItem *current, QListWidgetItem *previous);
    void onNodeTypeChanged(int index);

    // Modelling and utilities
    void onApplyClicked();
    void onClearClicked();
    void onCloseClicked();

private:
    // ---- Route algorithm ----

    /**
     * @brief Generates an orthogonal route between the current start and end nodes.
     *
     * Handles degenerate cases (collinear / co-planar), then evaluates all 6 axis-order
     * permutations (XYZ, XZY, …) and selects the shortest total-length path.
     * The result is written into m_nodes (start + waypoints + end).
     */
    void computeOrthogonalPath();

    /**
     * @brief Builds a TopoDS_Wire from the current node list.
     * @return The wire, or a null shape on failure.
     */
    TopoDS_Shape buildWire() const;

    /**
     * @brief Sweeps a rectangular cross-section along the wire to create a solid.
     * @param wire   Path wire.
     * @param width  Busbar width [mm].
     * @param thick  Busbar thickness [mm].
     * @param upDir  Reference "up" direction used to orient the section.
     * @return The resulting solid, or a null shape on failure.
     */
    TopoDS_Shape buildSolid(const TopoDS_Shape &wire,
                            double              width,
                            double              thick,
                            const gp_Dir       &upDir) const;

    // ---- UI helpers ----
    void refreshWaypointList();
    void updateNodePropertyPanel();
    void saveCurrentNodeProps();
    void updateStatus(const QString &msg);
    void clearPreview();

    /// Returns a display name for the given ConnectionType.
    static QString connectionTypeName(ConnectionType t);

    /// Maps a normal combo-box index (0-5) to a unit direction vector.
    static gp_Dir normalFromIndex(int idx);

    /// Default combo-box index for +Z normal.
    static int defaultNormalIndex() { return 4; }

private:
    Ui::WidgetBusbar *ui{};

    // ---- Pick helpers (one per endpoint) ----
    ViewerPickHelper *m_pickHelperStart{nullptr}; ///< Used while picking the start point
    ViewerPickHelper *m_pickHelperEnd{nullptr};   ///< Used while picking the end point

    bool   m_hasStart{false};
    bool   m_hasEnd{false};

    // ---- Route data ----
    std::vector<BusbarNode> m_nodes; ///< [0]=start, [1..n-2]=waypoints, [n-1]=end

    // ---- Currently selected node index (waypoints only) ----
    int m_selectedNodeIndex{-1};

    // ---- AIS handles for preview wire and final solid ----
    Handle(AIS_Shape) m_previewWire;
    Handle(AIS_Shape) m_solidShape;
};
