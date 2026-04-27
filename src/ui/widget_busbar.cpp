#include "widget_busbar.h"
#include "ui_widget_busbar.h"

#include "ViewManager.h"
#include "OCCView.h"
#include "ViewerPickHelper.h"

// OCC
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax2.hxx>
#include <gp_Vec.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS.hxx>
#include <AIS_InteractiveContext.hxx>
#include <Precision.hxx>

// Qt
#include <QMessageBox>
#include <QListWidgetItem>
#include <QCloseEvent>

// ============================================================
// Anonymous namespace - helpers
// ============================================================
namespace
{

/// Returns the total arc-length of a polyline defined by pts.
double pathLength(const std::vector<gp_Pnt> &pts)
{
    double len = 0.0;
    for (std::size_t i = 1; i < pts.size(); ++i)
        len += pts[i - 1].Distance(pts[i]);
    return len;
}

/**
 * @brief Generates one orthogonal candidate path using the given axis order.
 *
 * Starting from p0, the function moves sequentially along axes[0], axes[1],
 * axes[2] until it reaches p1.  This produces at most two intermediate
 * waypoints (four points total: p0, Q1, Q2, p1).
 *
 * @param p0    Start point.
 * @param p1    End point.
 * @param axes  Axis indices in traversal order (0=X, 1=Y, 2=Z).
 * @return      Candidate path points including p0 and p1.
 */
std::vector<gp_Pnt> makeCandidatePath(const gp_Pnt &p0,
                                      const gp_Pnt &p1,
                                      const int     axes[3])
{
    double c[3] = {p0.X(), p0.Y(), p0.Z()};
    const double d[3] = {p1.X(), p1.Y(), p1.Z()};

    std::vector<gp_Pnt> pts;
    pts.push_back(p0);

    for (int k = 0; k < 3; ++k) {
        int ax = axes[k];
        c[ax] = d[ax];
        pts.emplace_back(c[0], c[1], c[2]);
    }
    return pts;
}

/// Removes consecutive duplicate points closer than Precision::Confusion().
std::vector<gp_Pnt> removeDuplicates(const std::vector<gp_Pnt> &pts)
{
    std::vector<gp_Pnt> out;
    for (const auto &p : pts) {
        if (out.empty() || out.back().Distance(p) > Precision::Confusion())
            out.push_back(p);
    }
    return out;
}

} // namespace

// ============================================================
// WidgetBusbar
// ============================================================

WidgetBusbar::WidgetBusbar(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WidgetBusbar)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    // Default normal direction: +Z (index 4)
    ui->comboBoxStartNormal->setCurrentIndex(defaultNormalIndex());
    ui->comboBoxEndNormal->setCurrentIndex(defaultNormalIndex());

    // Button connections
    connect(ui->pushButtonPickStart,      &QPushButton::clicked, this, &WidgetBusbar::onPickStartClicked);
    connect(ui->pushButtonPickEnd,        &QPushButton::clicked, this, &WidgetBusbar::onPickEndClicked);
    connect(ui->pushButtonAutoCalc,       &QPushButton::clicked, this, &WidgetBusbar::onAutoCalcClicked);
    connect(ui->pushButtonAddWaypoint,    &QPushButton::clicked, this, &WidgetBusbar::onAddWaypointClicked);
    connect(ui->pushButtonDeleteWaypoint, &QPushButton::clicked, this, &WidgetBusbar::onDeleteWaypointClicked);
    connect(ui->pushButtonApply,          &QPushButton::clicked, this, &WidgetBusbar::onApplyClicked);
    connect(ui->pushButtonClear,          &QPushButton::clicked, this, &WidgetBusbar::onClearClicked);
    connect(ui->pushButtonClose,          &QPushButton::clicked, this, &WidgetBusbar::onCloseClicked);
    connect(ui->listWidgetWaypoints,      &QListWidget::currentItemChanged,
            this, &WidgetBusbar::onWaypointSelected);
    connect(ui->comboBoxNodeType,         QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &WidgetBusbar::onNodeTypeChanged);

    updateStatus(tr("Ready"));
}

WidgetBusbar::~WidgetBusbar()
{
    clearPreview();
    delete ui;
}

void WidgetBusbar::show()
{
    QWidget::show();
    refreshWaypointList();
}

void WidgetBusbar::closeEvent(QCloseEvent *event)
{
    clearPreview();
    QWidget::closeEvent(event);
}

// ============================================================
// Start point picking
// ============================================================

void WidgetBusbar::onPickStartClicked()
{
    // Stop any in-progress end-pick
    if (m_pickHelperEnd && m_pickHelperEnd->isActive())
        m_pickHelperEnd->stop();

    if (!m_pickHelperStart)
        m_pickHelperStart = new ViewerPickHelper(this);

    // Disconnect any previous connection to avoid duplicate slots
    disconnect(m_pickHelperStart, &ViewerPickHelper::pointPicked,
               this, &WidgetBusbar::onStartPicked);
    connect(m_pickHelperStart, &ViewerPickHelper::pointPicked,
            this, &WidgetBusbar::onStartPicked, Qt::UniqueConnection);

    m_pickHelperStart->start();
    updateStatus(tr("Click in the 3D view to pick the start point..."));
}

void WidgetBusbar::onStartPicked(double x, double y, double z)
{
    // One-shot: immediately disconnect so this slot fires only once
    if (m_pickHelperStart)
        disconnect(m_pickHelperStart, &ViewerPickHelper::pointPicked,
                   this, &WidgetBusbar::onStartPicked);
    if (m_pickHelperStart)
        m_pickHelperStart->stop();

    BusbarNode node;
    node.position   = gp_Pnt(x, y, z);
    node.normal     = normalFromIndex(ui->comboBoxStartNormal->currentIndex());
    node.isEndpoint = true;

    // Start node is always m_nodes[0]
    if (m_hasStart && !m_nodes.empty())
        m_nodes[0] = node;
    else {
        m_nodes.insert(m_nodes.begin(), node);
        m_hasStart = true;
    }

    ui->labelStartPosValue->setText(
        QString("(%1, %2, %3)").arg(x, 0, 'f', 1).arg(y, 0, 'f', 1).arg(z, 0, 'f', 1));
    updateStatus(tr("Start point picked."));
    refreshWaypointList();
}

// ============================================================
// End point picking
// ============================================================

void WidgetBusbar::onPickEndClicked()
{
    // Stop any in-progress start-pick
    if (m_pickHelperStart && m_pickHelperStart->isActive())
        m_pickHelperStart->stop();

    if (!m_pickHelperEnd)
        m_pickHelperEnd = new ViewerPickHelper(this);

    disconnect(m_pickHelperEnd, &ViewerPickHelper::pointPicked,
               this, &WidgetBusbar::onEndPicked);
    connect(m_pickHelperEnd, &ViewerPickHelper::pointPicked,
            this, &WidgetBusbar::onEndPicked, Qt::UniqueConnection);

    m_pickHelperEnd->start();
    updateStatus(tr("Click in the 3D view to pick the end point..."));
}

void WidgetBusbar::onEndPicked(double x, double y, double z)
{
    // One-shot: immediately disconnect so this slot fires only once
    if (m_pickHelperEnd)
        disconnect(m_pickHelperEnd, &ViewerPickHelper::pointPicked,
                   this, &WidgetBusbar::onEndPicked);
    if (m_pickHelperEnd)
        m_pickHelperEnd->stop();

    BusbarNode node;
    node.position   = gp_Pnt(x, y, z);
    node.normal     = normalFromIndex(ui->comboBoxEndNormal->currentIndex());
    node.isEndpoint = true;

    if (m_hasEnd && !m_nodes.empty())
        m_nodes.back() = node;
    else {
        m_nodes.push_back(node);
        m_hasEnd = true;
    }

    ui->labelEndPosValue->setText(
        QString("(%1, %2, %3)").arg(x, 0, 'f', 1).arg(y, 0, 'f', 1).arg(z, 0, 'f', 1));
    updateStatus(tr("End point picked."));
    refreshWaypointList();
}

// ============================================================
// Auto route calculation
// ============================================================

void WidgetBusbar::onAutoCalcClicked()
{
    if (!m_hasStart || !m_hasEnd || m_nodes.size() < 2) {
        updateStatus(tr("Please pick start and end points first."));
        return;
    }
    computeOrthogonalPath();
    refreshWaypointList();

    // Display preview wire
    clearPreview();
    TopoDS_Shape wire = buildWire();
    if (!wire.IsNull()) {
        auto view = ViewManager::getInstance().getActiveView();
        if (view && view->Context()) {
            m_previewWire = new AIS_Shape(wire);
            m_previewWire->SetColor(Quantity_NOC_CYAN1);
            view->Context()->Display(m_previewWire, Standard_True);
        }
    }
    updateStatus(tr("Route calculated automatically."));
}

void WidgetBusbar::computeOrthogonalPath()
{
    // Preserve start and end; clear intermediate waypoints
    BusbarNode startNode = m_nodes.front();
    BusbarNode endNode   = m_nodes.back();

    const gp_Pnt &p0 = startNode.position;
    const gp_Pnt &p1 = endNode.position;

    // Resolve default connection type from radio buttons
    ConnectionType defConn = ConnectionType::Bend;
    if (ui->radioButtonBolt->isChecked()) defConn = ConnectionType::Bolt;
    if (ui->radioButtonWeld->isChecked()) defConn = ConnectionType::Weld;

    // Count non-zero displacement axes
    double dx = std::abs(p1.X() - p0.X());
    double dy = std::abs(p1.Y() - p0.Y());
    double dz = std::abs(p1.Z() - p0.Z());
    int nonZeroAxes = (dx > Precision::Confusion() ? 1 : 0)
                    + (dy > Precision::Confusion() ? 1 : 0)
                    + (dz > Precision::Confusion() ? 1 : 0);

    m_nodes.clear();
    m_nodes.push_back(startNode);

    if (nonZeroAxes <= 1) {
        // Collinear: straight segment, no waypoints
    } else if (nonZeroAxes == 2) {
        // Co-planar: L-shaped path with one waypoint
        double c[3] = {p0.X(), p0.Y(), p0.Z()};
        double d[3] = {p1.X(), p1.Y(), p1.Z()};
        int firstAxis = (dx > Precision::Confusion()) ? 0 : (dy > Precision::Confusion() ? 1 : 2);
        c[firstAxis] = d[firstAxis];

        BusbarNode wp;
        wp.position = gp_Pnt(c[0], c[1], c[2]);
        wp.type     = defConn;
        m_nodes.push_back(wp);
    } else {
        // General case: enumerate all 6 axis-order permutations, pick shortest
        const int perms[6][3] = {
            {0, 1, 2}, {0, 2, 1},
            {1, 0, 2}, {1, 2, 0},
            {2, 0, 1}, {2, 1, 0}
        };

        std::vector<gp_Pnt> bestPts;
        double bestLen = std::numeric_limits<double>::max();

        for (const auto &axes : perms) {
            auto cand = makeCandidatePath(p0, p1, axes);
            cand = removeDuplicates(cand);
            double len = pathLength(cand);
            if (len < bestLen) {
                bestLen = len;
                bestPts = cand;
            }
        }

        // Insert intermediate points as waypoints (exclude first and last)
        for (std::size_t i = 1; i + 1 < bestPts.size(); ++i) {
            BusbarNode wp;
            wp.position = bestPts[i];
            wp.type     = defConn;
            m_nodes.push_back(wp);
        }
    }

    m_nodes.push_back(endNode);
}

// ============================================================
// Waypoint add / delete
// ============================================================

void WidgetBusbar::onAddWaypointClicked()
{
    if (!m_hasStart || !m_hasEnd || m_nodes.size() < 2) {
        updateStatus(tr("Pick start and end points before adding waypoints."));
        return;
    }

    // Insert a new waypoint at the midpoint of the last segment
    BusbarNode &last = m_nodes.back();
    BusbarNode &prev = m_nodes[m_nodes.size() - 2];

    BusbarNode wp;
    wp.position = gp_Pnt(
        (prev.position.X() + last.position.X()) * 0.5,
        (prev.position.Y() + last.position.Y()) * 0.5,
        (prev.position.Z() + last.position.Z()) * 0.5);
    wp.type = ConnectionType::Bend;

    m_nodes.insert(m_nodes.end() - 1, wp);
    refreshWaypointList();
    updateStatus(tr("Waypoint added at midpoint of last segment."));
}

void WidgetBusbar::onDeleteWaypointClicked()
{
    const int row = ui->listWidgetWaypoints->currentRow();
    if (row < 0 || row >= static_cast<int>(m_nodes.size())) return;
    if (m_nodes[row].isEndpoint) {
        updateStatus(tr("Start and end points cannot be deleted."));
        return;
    }
    m_nodes.erase(m_nodes.begin() + row);
    m_selectedNodeIndex = -1;
    refreshWaypointList();
    updateStatus(tr("Waypoint deleted."));
}

// ============================================================
// Waypoint list & property panel
// ============================================================

void WidgetBusbar::refreshWaypointList()
{
    ui->listWidgetWaypoints->blockSignals(true);
    ui->listWidgetWaypoints->clear();

    for (std::size_t i = 0; i < m_nodes.size(); ++i) {
        const BusbarNode &n = m_nodes[i];
        QString label;
        if (i == 0)
            label = tr("P0  Start");
        else if (i == m_nodes.size() - 1)
            label = QString("P%1  End").arg(i);
        else
            label = QString("P%1  Waypoint [%2]").arg(i).arg(connectionTypeName(n.type));

        label += QString("  (%1, %2, %3)")
                     .arg(n.position.X(), 0, 'f', 1)
                     .arg(n.position.Y(), 0, 'f', 1)
                     .arg(n.position.Z(), 0, 'f', 1);

        ui->listWidgetWaypoints->addItem(label);
    }
    ui->listWidgetWaypoints->blockSignals(false);
    updateNodePropertyPanel();
}

void WidgetBusbar::onWaypointSelected(QListWidgetItem *current, QListWidgetItem * /*previous*/)
{
    if (!current) { m_selectedNodeIndex = -1; updateNodePropertyPanel(); return; }

    // Save previously edited node properties back to m_nodes
    if (m_selectedNodeIndex >= 0 && m_selectedNodeIndex < static_cast<int>(m_nodes.size()))
        saveCurrentNodeProps();

    m_selectedNodeIndex = ui->listWidgetWaypoints->row(current);
    updateNodePropertyPanel();
}

void WidgetBusbar::updateNodePropertyPanel()
{
    // Only intermediate waypoints are editable
    bool editable = (m_selectedNodeIndex > 0 &&
                     m_selectedNodeIndex < static_cast<int>(m_nodes.size()) - 1);

    ui->groupBoxNodeProp->setEnabled(editable);
    if (!editable) return;

    const BusbarNode &n = m_nodes[m_selectedNodeIndex];
    ui->comboBoxNodeType->blockSignals(true);
    ui->comboBoxNodeType->setCurrentIndex(static_cast<int>(n.type));
    ui->comboBoxNodeType->blockSignals(false);

    ui->spinBoxBendRadius->setValue(n.bend.radius);
    ui->comboBoxBoltSpec->setCurrentIndex(n.bolt.specIndex);
    ui->comboBoxWeldType->setCurrentIndex(n.weld.typeIndex);

    onNodeTypeChanged(static_cast<int>(n.type));
}

void WidgetBusbar::onNodeTypeChanged(int index)
{
    // Show only the parameters relevant to the selected connection type
    bool isBend = (index == 0);
    bool isBolt = (index == 1);
    bool isWeld = (index == 2);

    ui->labelBendRadius->setVisible(isBend);
    ui->spinBoxBendRadius->setVisible(isBend);
    ui->labelBoltSpec->setVisible(isBolt);
    ui->comboBoxBoltSpec->setVisible(isBolt);
    ui->labelWeldType->setVisible(isWeld);
    ui->comboBoxWeldType->setVisible(isWeld);
}

void WidgetBusbar::saveCurrentNodeProps()
{
    if (m_selectedNodeIndex < 0 || m_selectedNodeIndex >= static_cast<int>(m_nodes.size())) return;
    BusbarNode &n      = m_nodes[m_selectedNodeIndex];
    n.type             = static_cast<ConnectionType>(ui->comboBoxNodeType->currentIndex());
    n.bend.radius      = ui->spinBoxBendRadius->value();
    n.bolt.specIndex   = ui->comboBoxBoltSpec->currentIndex();
    n.weld.typeIndex   = ui->comboBoxWeldType->currentIndex();
}

// ============================================================
// Apply - generate 3-D busbar solid
// ============================================================

void WidgetBusbar::onApplyClicked()
{
    if (m_nodes.size() < 2) {
        updateStatus(tr("At least a start and end point are required."));
        return;
    }

    // Commit current property panel before building
    if (m_selectedNodeIndex >= 0)
        saveCurrentNodeProps();

    TopoDS_Shape wire = buildWire();
    if (wire.IsNull()) {
        updateStatus(tr("Failed to build route wire."));
        return;
    }

    // Determine cross-section up-direction
    gp_Dir upDir(0, 0, 1);
    int sectionIdx = ui->comboBoxSectionDir->currentIndex();
    if (sectionIdx == 1) upDir = gp_Dir(1, 0, 0);
    else if (sectionIdx == 2) upDir = gp_Dir(0, 1, 0);
    // sectionIdx == 0 → auto, keep (0,0,1); sectionIdx == 3 → Fixed +Z, same

    double w = ui->spinBoxWidth->value();
    double t = ui->spinBoxThickness->value();

    TopoDS_Shape solid = buildSolid(wire, w, t, upDir);
    if (solid.IsNull()) {
        updateStatus(tr("Failed to generate busbar solid."));
        return;
    }

    auto view = ViewManager::getInstance().getActiveView();
    if (!view) { updateStatus(tr("No active view available.")); return; }

    // Remove previous solid if any
    if (!m_solidShape.IsNull()) {
        view->removeShape(m_solidShape->Shape());
        m_solidShape.Nullify();
    }

    m_solidShape = new AIS_Shape(solid);
    m_solidShape->SetColor(Quantity_NOC_GOLD);
    // Let OCCView manage this shape so it responds to setDisplayMode
    view->setShape(m_solidShape);
    view->reDraw();

    updateStatus(tr("Busbar solid generated successfully."));
}

// ============================================================
// Clear
// ============================================================

void WidgetBusbar::onClearClicked()
{
    clearPreview();
    m_nodes.clear();
    m_hasStart = false;
    m_hasEnd   = false;
    m_selectedNodeIndex = -1;

    ui->labelStartPosValue->setText(tr("-"));
    ui->labelEndPosValue->setText(tr("-"));
    ui->listWidgetWaypoints->clear();
    updateStatus(tr("Cleared."));
}

void WidgetBusbar::onCloseClicked()
{
    close();
}

void WidgetBusbar::clearPreview()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view || !view->Context()) return;

    if (!m_previewWire.IsNull()) {
        view->Context()->Remove(m_previewWire, Standard_False);
        m_previewWire.Nullify();
    }
    if (!m_solidShape.IsNull()) {
        view->removeShape(m_solidShape->Shape());
        m_solidShape.Nullify();
    }
    view->reDraw();
    view->Context()->UpdateCurrentViewer();
}

// ============================================================
// Route wire construction
// ============================================================

TopoDS_Shape WidgetBusbar::buildWire() const
{
    if (m_nodes.size() < 2) return {};

    BRepBuilderAPI_MakeWire wireBuilder;
    bool ok = false;

    for (std::size_t i = 0; i + 1 < m_nodes.size(); ++i) {
        const gp_Pnt &a = m_nodes[i].position;
        const gp_Pnt &b = m_nodes[i + 1].position;

        if (a.Distance(b) < Precision::Confusion()) continue;

        BRepBuilderAPI_MakeEdge edgeMaker(a, b);
        if (!edgeMaker.IsDone()) continue;

        wireBuilder.Add(edgeMaker.Edge());
        ok = true;
    }

    if (!ok || !wireBuilder.IsDone()) return {};
    return wireBuilder.Wire();
}

// ============================================================
// Sweep solid construction
// ============================================================

TopoDS_Shape WidgetBusbar::buildSolid(const TopoDS_Shape &/*wireShape*/,
                                      double              width,
                                      double              thick,
                                      const gp_Dir       &upDir) const
{
    // Strategy: per-segment Prism + Fuse.
    // BRepOffsetAPI_MakePipe fails at 90-degree corners on orthogonal routes;
    // extruding each segment independently and fusing the results is robust.
    try {
        if (m_nodes.size() < 2) return {};

        TopoDS_Shape result;
        bool firstSeg = true;

        for (std::size_t i = 0; i + 1 < m_nodes.size(); ++i) {
            const gp_Pnt &a = m_nodes[i].position;
            const gp_Pnt &b = m_nodes[i + 1].position;

            gp_Vec segVec(a, b);
            double segLen = segVec.Magnitude();
            if (segLen < Precision::Confusion()) continue;
            segVec.Normalize();

            // Build a local frame for the cross-section at point 'a':
            //   X = segment direction
            //   Y = section width axis  (perpendicular to X, guided by upDir)
            //   Z = section thickness axis
            gp_Dir xDir(segVec);
            gp_Dir zRef = upDir;
            // If segment is parallel to upDir, choose an orthogonal fallback
            if (std::abs(gp_Vec(zRef).Dot(segVec)) > 0.999)
                zRef = (std::abs(segVec.Y()) < 0.999) ? gp_Dir(0, 1, 0)
                                                       : gp_Dir(1, 0, 0);

            gp_Vec yVec = gp_Vec(zRef).Crossed(segVec).Normalized();
            gp_Vec zVec = segVec.Crossed(yVec).Normalized();

            // Four corners of the rectangular cross-section at start point 'a'
            double hw = width * 0.5;
            double ht = thick * 0.5;

            gp_Pnt c0 = a.Translated( hw * yVec + ht * zVec);
            gp_Pnt c1 = a.Translated(-hw * yVec + ht * zVec);
            gp_Pnt c2 = a.Translated(-hw * yVec - ht * zVec);
            gp_Pnt c3 = a.Translated( hw * yVec - ht * zVec);

            BRepBuilderAPI_MakePolygon rect(c0, c1, c2, c3, Standard_True);
            if (!rect.IsDone()) continue;

            BRepBuilderAPI_MakeFace face(rect.Wire(), Standard_True);
            if (!face.IsDone()) continue;

            // Extrude the face along the segment vector to produce a solid
            gp_Vec extrudeVec = segVec * segLen;
            BRepPrimAPI_MakePrism prism(face.Face(), extrudeVec, Standard_False);
            prism.Build();
            if (!prism.IsDone()) continue;

            TopoDS_Shape segSolid = prism.Shape();
            if (segSolid.IsNull()) continue;

            if (firstSeg) {
                result   = segSolid;
                firstSeg = false;
            } else {
                // Fuse this segment's box with the accumulated solid
                BRepAlgoAPI_Fuse fuse(result, segSolid);
                fuse.Build();
                if (fuse.IsDone())
                    result = fuse.Shape();
                else
                    result = segSolid; // fall back: keep at least current segment
            }
        }

        return result;
    } catch (...) {
        return {};
    }
}

// ============================================================
// Utility
// ============================================================

void WidgetBusbar::updateStatus(const QString &msg)
{
    ui->labelStatus->setText(msg);
}

QString WidgetBusbar::connectionTypeName(ConnectionType t)
{
    switch (t) {
    case ConnectionType::Bend: return QStringLiteral("Bend");
    case ConnectionType::Bolt: return QStringLiteral("Bolt");
    case ConnectionType::Weld: return QStringLiteral("Weld");
    }
    return QStringLiteral("Bend");
}

gp_Dir WidgetBusbar::normalFromIndex(int idx)
{
    switch (idx) {
    case 0: return gp_Dir( 1,  0,  0);
    case 1: return gp_Dir(-1,  0,  0);
    case 2: return gp_Dir( 0,  1,  0);
    case 3: return gp_Dir( 0, -1,  0);
    case 4: return gp_Dir( 0,  0,  1);
    case 5: return gp_Dir( 0,  0, -1);
    default: return gp_Dir(0,  0,  1);
    }
}
