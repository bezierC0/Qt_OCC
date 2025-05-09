#include "TreeWidget.h"

#include "qstring_conv.h"
#include <TDataStd_Name.hxx>
#include <unordered_set>
#include <XCAFDoc_ShapeTool.hxx>

TreeWidget::TreeWidget( QWidget* parent )
  : QTreeWidget( parent )
{
    setColumnCount( 1 );
    setHeaderLabel( "Assembly Tree" );

    //populateWithDummyData(); 
}

QModelIndex TreeWidget::indexFromItem( QTreeWidgetItem* item, const int column ) const
{
    return QTreeWidget::indexFromItem( item, column );
}

QTreeWidgetItem* TreeWidget::itemFromIndex( const QModelIndex& index ) const
{
    return QTreeWidget::itemFromIndex( index );
}

void TreeWidget::setData( const Tree<TDF_Label>& modelTree )
{
    rebuildData( modelTree );
}

void TreeWidget::rebuildData( const Tree<TDF_Label>& modelTree )
{
    // const TCollection_ExtendedString& CafUtils::labelAttrStdName(const TDF_Label& label)
    auto labelAttrStdName = []( const TDF_Label& label )
    {
        Handle_TDataStd_Name attrName;
        if ( label.FindAttribute( TDataStd_Name::GetID(), attrName ) ) {
            return attrName->Get();
        }
        else {
            static const TCollection_ExtendedString nullStr = {};
            return nullStr;
        }
    };
    for ( const auto& rootId : modelTree.m_vecRoot ) {
        const TDF_Label& label = modelTree.nodeData( rootId );
        QTreeWidgetItem* rootItem = new QTreeWidgetItem( this );
        const QString instanceName = Mayo::to_QString( labelAttrStdName( label ) );
        rootItem->setText( 0, QString::fromStdString( std::to_string( rootId ) + " : " + instanceName.toStdString() ) );

        buildSubTree( modelTree, rootId, rootItem );
    }
}
std::unordered_map<TreeNodeId, QTreeWidgetItem*> mapNodeIdToTreeItem;
std::unordered_set<TreeNodeId> setReferenceNodeId;
void TreeWidget::buildSubTree( const Tree<TDF_Label>& tree, const TreeNodeId parentId, QTreeWidgetItem* parentItem )
{
    // const TCollection_ExtendedString& CafUtils::labelAttrStdName(const TDF_Label& label)
    auto labelAttrStdName = []( const TDF_Label& label )
    {
        Handle_TDataStd_Name attrName;
        if ( label.FindAttribute( TDataStd_Name::GetID(), attrName ) ) {
            return attrName->Get();
        }
        else {
            static const TCollection_ExtendedString nullStr = {};
            return nullStr;
        }
    };

    // void WidgetModelTreeBuilder_Xde::refreshXdeAssemblyNodeItemText(QTreeWidgetItem* item)
    auto referenceItemText = [&]( const TDF_Label& instanceLabel, const TDF_Label& productLabel )->QString
    {
        const QString instanceName = Mayo::to_QString( labelAttrStdName( instanceLabel ) );
        const QString productName = Mayo::to_QString( labelAttrStdName( productLabel ) );
        //const QByteArray strTemplate = Module::toInstanceNameTemplate( Module::get()->instanceNameFormat );
        //QString itemText = QString::fromUtf8( strTemplate );
        //itemText.replace( "%instance", instanceName )
        //  .replace( "%product", productName );
        return instanceName + productName ;
    };

    auto shapeReferred = []( const TDF_Label& lbl ) -> TDF_Label
    {
        TDF_Label referred;
        XCAFDoc_ShapeTool::GetReferredShape( lbl, referred );
        return referred;
    };

    // QString WidgetModelTreeBuilder_Xde::referenceItemText(
    TreeNodeId childId = tree.nodeChildFirst( parentId );
    while ( childId != 0 ) {
      
        const TDF_Label& childLabel = tree.nodeData( childId );
        //const TDF_Label productLabel = shapeReferred( childLabel );
        QTreeWidgetItem* childItem = new QTreeWidgetItem( parentItem );
        const auto instanceName = Mayo::to_QString( labelAttrStdName( childLabel ) );
        //childItem->setText( 0, referenceItemText( childLabel , productLabel ) );
        childItem->setText( 0, QString::fromStdString( std::to_string( childId ) + " : " + instanceName.toStdString() ) );
        if ( XCAFDoc_ShapeTool::IsReference( childLabel ) ) {
            
        }
        else
        {
            buildSubTree( tree, childId, childItem );

        }
        childId = tree.nodeSiblingNext( childId );
    }
}
