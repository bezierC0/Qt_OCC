#pragma once

#include <QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QHeaderView>

class TopoDS_Shape;
class TDF_Label;
template<typename T>
class Tree;

class TreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit TreeWidget( QWidget* parent = nullptr ) ;

    QModelIndex indexFromItem( QTreeWidgetItem* item, const int column = 0 ) const;

    QTreeWidgetItem* itemFromIndex( const QModelIndex& index ) const;

    void setData( const Tree<TDF_Label>& modelTree );
    
signals:
    void labelSelected(const TDF_Label& label);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);

private:
    void rebuildData( const Tree<TDF_Label>& modelTree );
    void buildAssemblyTree( const Tree<TDF_Label>& tree, uint32_t parentId, QTreeWidgetItem* parentItem );
    void buildShapeTree(const TopoDS_Shape& shape, const QTreeWidgetItem* parentItem );

    std::shared_ptr<Tree<TDF_Label>> m_modelTree = nullptr;
};
