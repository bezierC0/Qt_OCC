#include "ModelTreeContextMenu.h"

ModelTreeContextMenu::ModelTreeContextMenu(QWidget* parent)
    : QMenu(parent)
{
    // Pick action
    m_actionPick = addAction(tr("Pick (Select)"));
    connect(m_actionPick, &QAction::triggered, this, &ModelTreeContextMenu::onPickTriggered);

    // Remove action
    m_actionRemove = addAction(tr("Remove"));
    connect(m_actionRemove, &QAction::triggered, this, &ModelTreeContextMenu::onRemoveTriggered);

    // Separator
    addSeparator();

    // Filter submenu
    QMenu* filterMenu = addMenu(tr("Filter"));

    m_actionFilterVertex = filterMenu->addAction(tr("Vertex"));
    m_actionFilterEdge   = filterMenu->addAction(tr("Edge"));
    m_actionFilterFace   = filterMenu->addAction(tr("Face"));

    m_actionFilterVertex->setCheckable(true);
    m_actionFilterEdge->setCheckable(true);
    m_actionFilterFace->setCheckable(true);
    m_actionFilterVertex->setChecked(true);  // Default: show all levels

    QActionGroup* filterGroup = new QActionGroup(this);
    filterGroup->setExclusive(true);
    filterGroup->addAction(m_actionFilterVertex);
    filterGroup->addAction(m_actionFilterEdge);
    filterGroup->addAction(m_actionFilterFace);

    connect(m_actionFilterVertex, &QAction::triggered, this, &ModelTreeContextMenu::onFilterVertexTriggered);
    connect(m_actionFilterEdge,   &QAction::triggered, this, &ModelTreeContextMenu::onFilterEdgeTriggered);
    connect(m_actionFilterFace,   &QAction::triggered, this, &ModelTreeContextMenu::onFilterFaceTriggered);
}

void ModelTreeContextMenu::popup(const QPoint& globalPos, bool actionsEnabled)
{
    m_actionPick->setEnabled(actionsEnabled);
    m_actionRemove->setEnabled(actionsEnabled);
    exec(globalPos);
}

void ModelTreeContextMenu::setFilterLevel(TreeFilterLevel level)
{
    m_filterLevel = level;
    m_actionFilterVertex->setChecked(level == TreeFilterLevel::Vertex);
    m_actionFilterEdge->setChecked(level == TreeFilterLevel::Edge);
    m_actionFilterFace->setChecked(level == TreeFilterLevel::Face);
}

void ModelTreeContextMenu::onPickTriggered()
{
    emit pickRequested();
}

void ModelTreeContextMenu::onRemoveTriggered()
{
    emit removeRequested();
}

void ModelTreeContextMenu::onFilterVertexTriggered()
{
    m_filterLevel = TreeFilterLevel::Vertex;
    emit filterLevelChanged(m_filterLevel);
}

void ModelTreeContextMenu::onFilterEdgeTriggered()
{
    m_filterLevel = TreeFilterLevel::Edge;
    emit filterLevelChanged(m_filterLevel);
}

void ModelTreeContextMenu::onFilterFaceTriggered()
{
    m_filterLevel = TreeFilterLevel::Face;
    emit filterLevelChanged(m_filterLevel);
}
