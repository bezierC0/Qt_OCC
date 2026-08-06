#pragma once

#include <QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QHeaderView>
#include <QMouseEvent>
#include "ModelTreeContextMenu.h"

class TopoDS_Shape;
class TDF_Label;
template<typename T>
class Tree;

class TreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit TreeWidget( QWidget* parent = nullptr ) ;

    void mousePressEvent(QMouseEvent *event) override;

    QModelIndex indexFromItem( QTreeWidgetItem* item, const int column = 0 ) const;

    QTreeWidgetItem* itemFromIndex( const QModelIndex& index ) const;

    void setData( const Tree<TDF_Label>& modelTree );
    std::shared_ptr<Tree<TDF_Label>> getModelTree() const { return m_modelTree; }

    /**
     * @brief Set the filter level for tree display depth.
     * @param level Filter level (Vertex, Edge, Face)
     */
    void setFilterLevel(TreeFilterLevel level);

    /**
     * @brief Get the current filter level.
     */
    TreeFilterLevel filterLevel() const { return m_filterLevel; }

signals:
    void labelSelected(const TDF_Label& label);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);

private:
    void rebuildData( const Tree<TDF_Label>& modelTree );
    void buildAssemblyTree( const Tree<TDF_Label>& tree, uint32_t parentId, QTreeWidgetItem* parentItem );
    void buildShapeTree(const Tree<TDF_Label>& tree, const TopoDS_Shape& shape, QTreeWidgetItem* parentItem );

    std::shared_ptr<Tree<TDF_Label>> m_modelTree = nullptr;
    TreeFilterLevel m_filterLevel = TreeFilterLevel::Vertex;
};
