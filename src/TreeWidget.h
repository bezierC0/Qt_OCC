#pragma once
#include <QVBoxLayout>
#include <TDF_Label.hxx>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QHeaderView>

#include "Tree.h"

class TreeWidget : public QTreeWidget
{
public:
    explicit TreeWidget( QWidget* parent = nullptr ) ;

    QModelIndex indexFromItem( QTreeWidgetItem* item, const int column = 0 ) const;

    QTreeWidgetItem* itemFromIndex( const QModelIndex& index ) const;

    void setData( const Tree<TDF_Label>& modelTree );

private:
    void rebuildData( const Tree<TDF_Label>& modelTree );
    void buildSubTree( const Tree<TDF_Label>& tree, TreeNodeId parentId, QTreeWidgetItem* parentItem );
};
