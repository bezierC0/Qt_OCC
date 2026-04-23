#include "WidgetModelTree.h"
#include "ModelTreeContextMenu.h"
#include "TreeWidget.h"
#include "Tree.h"

#include <TDF_Label.hxx>
#include <XCAFDoc_ShapeTool.hxx>

#include <QVBoxLayout>


ModelTreeWidget::ModelTreeWidget(QWidget* widget)
    : QWidget(widget)
{
    m_treeWidget  = new TreeWidget(this);
    m_contextMenu = new ModelTreeContextMenu(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_treeWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    // Forward label-selected signal from tree widget
    connect(m_treeWidget, &TreeWidget::labelSelected,
            this, &ModelTreeWidget::labelSelected);

    // Show context menu on right-click (logic layer)
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeWidget, &QWidget::customContextMenuRequested,
            this, &ModelTreeWidget::onContextMenuRequested);

    // Forward pick / remove signals from context menu view layer
    connect(m_contextMenu, &ModelTreeContextMenu::pickRequested,
            this, &ModelTreeWidget::onContextMenuPick);
    connect(m_contextMenu, &ModelTreeContextMenu::removeRequested,
            this, &ModelTreeWidget::onContextMenuRemove);
}


ModelTreeWidget::~ModelTreeWidget()
= default;

void ModelTreeWidget::setModelTree(const Tree<TDF_Label>& modelTree)
{
    m_treeWidget->setData(modelTree);
}

// ── Controller: determine label validity, delegate display to the View ────────
void ModelTreeWidget::onContextMenuRequested(const QPoint& pos)
{
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    if (!item)
        return;

    bool ok = false;
    const auto nodeId = item->data(0, Qt::UserRole).toUInt(&ok);
    if (!ok)
        return;

    const auto modelTree = m_treeWidget->getModelTree();
    if (!modelTree)
        return;

    m_pendingLabel = std::make_unique<TDF_Label>(modelTree->nodeData(nodeId));
    if (m_pendingLabel->IsNull())
        return;

    // Actions are enabled only when the label represents a Shape or Part
    const bool isValidTarget = XCAFDoc_ShapeTool::IsShape(*m_pendingLabel)       ||
                               XCAFDoc_ShapeTool::IsSimpleShape(*m_pendingLabel) ||
                               XCAFDoc_ShapeTool::IsComponent(*m_pendingLabel);

    // Convert viewport coordinates to global screen coordinates, then show the View
    const QPoint globalPos = m_treeWidget->viewport()->mapToGlobal(pos);
    m_contextMenu->popup(globalPos, isValidTarget);
}

// ── Signal relay: translate View signals into business signals ────────────────
void ModelTreeWidget::onContextMenuPick()
{
    if (m_pendingLabel && !m_pendingLabel->IsNull())
        emit labelPickRequested(*m_pendingLabel);
}

void ModelTreeWidget::onContextMenuRemove()
{
    if (m_pendingLabel && !m_pendingLabel->IsNull())
        emit labelRemoveRequested(*m_pendingLabel);
}
