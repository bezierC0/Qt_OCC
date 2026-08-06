#pragma once

#include <QMenu>
#include <QPoint>

/**
 * @brief Filter level for assembly tree display depth.
 */
enum class TreeFilterLevel {
    Vertex,  // Show all levels including vertices
    Edge,    // Stop at edge level
    Face     // Stop at face level
};

/**
 * @brief Context menu for the Model Tree.
 */
class ModelTreeContextMenu : public QMenu
{
    Q_OBJECT
public:
    explicit ModelTreeContextMenu(QWidget* parent = nullptr);
    ~ModelTreeContextMenu() override = default;

    /**
     * @brief Display the menu at the given screen position.
     * @param globalPos      Screen coordinates.
     * @param actionsEnabled Whether the Pick / Remove actions should be enabled.
     */
    void popup(const QPoint& globalPos, bool actionsEnabled);

    /**
     * @brief Set the current filter level (updates checkbox state).
     */
    void setFilterLevel(TreeFilterLevel level);

    /**
     * @brief Get the current filter level.
     */
    TreeFilterLevel filterLevel() const { return m_filterLevel; }

signals:
    void pickRequested();
    void removeRequested();
    void filterLevelChanged(TreeFilterLevel level);

private slots:
    void onPickTriggered();
    void onRemoveTriggered();
    void onFilterVertexTriggered();
    void onFilterEdgeTriggered();
    void onFilterFaceTriggered();

private:
    QAction* m_actionPick = nullptr;
    QAction* m_actionRemove = nullptr;
    QAction* m_actionFilterVertex = nullptr;
    QAction* m_actionFilterEdge = nullptr;
    QAction* m_actionFilterFace = nullptr;
    TreeFilterLevel m_filterLevel = TreeFilterLevel::Vertex;
};
