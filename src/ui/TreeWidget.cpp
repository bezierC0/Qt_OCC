#include "TreeWidget.h"
#include "QStringConv.h"
#include "Tree.h"
#include <unordered_set>

#include <TDataStd_Name.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS.hxx>
#include <TDF_Label.hxx>
#include <BRep_Tool.hxx>

namespace
{
    // const TCollection_ExtendedString& CafUtils::labelAttrStdName(const TDF_Label& label)
    TCollection_ExtendedString labelAttrStdName(const TDF_Label& label)
    {
        Handle_TDataStd_Name attrName;
        if (label.FindAttribute(TDataStd_Name::GetID(), attrName))
        {
            return attrName->Get();
        }
        else
        {
            static const TCollection_ExtendedString nullStr = {};
            return nullStr;
        }
    }

    // void WidgetModelTreeBuilder_Xde::refreshXdeAssemblyNodeItemText(QTreeWidgetItem* item)
    QString referenceItemText(const TDF_Label& instanceLabel, const TDF_Label& productLabel)
    {
        const QString instanceName = Mayo::to_QString(labelAttrStdName(instanceLabel));
        const QString productName = Mayo::to_QString(labelAttrStdName(productLabel));
        //const QByteArray strTemplate = Module::toInstanceNameTemplate( Module::get()->instanceNameFormat );
        //QString itemText = QString::fromUtf8( strTemplate );
        //itemText.replace( "%instance", instanceName )
        //  .replace( "%product", productName );
        return instanceName + productName;
    }

    TDF_Label shapeReferred(const TDF_Label& lbl)
    {
        TDF_Label referred;
        XCAFDoc_ShapeTool::GetReferredShape(lbl, referred);
        return referred;
    }
}

TreeWidget::TreeWidget( QWidget* parent )
  : QTreeWidget( parent )
{
    setColumnCount( 1 );
    setHeaderLabel( "Assembly Tree" );

    connect(this, &QTreeWidget::itemClicked, this, &TreeWidget::onItemClicked);
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
    clear();
    m_modelTree = std::make_shared<Tree<TDF_Label>>( modelTree );
    rebuildData( *m_modelTree );
}

void TreeWidget::rebuildData( const Tree<TDF_Label>& modelTree )
{

    for ( const auto& rootId : modelTree.m_vecRoot ) {
        const TDF_Label& label = modelTree.nodeData( rootId );
        QTreeWidgetItem* rootItem = new QTreeWidgetItem( this );
        const QString instanceName = Mayo::to_QString( labelAttrStdName( label ) );
        rootItem->setText( 0, QString::fromStdString( std::to_string( rootId ) + " : " + instanceName.toStdString() ) );
        rootItem->setData(0, Qt::UserRole, QVariant::fromValue(rootId)); // Store TreeNodeId

        buildAssemblyTree( modelTree, rootId, rootItem );
    }
}

void TreeWidget::buildAssemblyTree( const Tree<TDF_Label>& tree, const TreeNodeId parentId, QTreeWidgetItem* parentItem )
{
    // QString WidgetModelTreeBuilder_Xde::referenceItemText(
    TreeNodeId childId = tree.nodeChildFirst( parentId );
    while ( childId != 0 ) {
        const TDF_Label& childLabel = tree.nodeData( childId );
        //const TDF_Label productLabel = shapeReferred( childLabel );
        auto* childItem = new QTreeWidgetItem( parentItem );
        const auto instanceName = Mayo::to_QString( labelAttrStdName( childLabel ) );
        //childItem->setText( 0, referenceItemText( childLabel , productLabel ) );
        childItem->setText( 0, QString::fromStdString( std::to_string( childId ) + " : " + instanceName.toStdString() ) );
        childItem->setData(0, Qt::UserRole, QVariant::fromValue(childId)); // Store TreeNodeId
        if ( XCAFDoc_ShapeTool::IsReference( childLabel ) ) 
        {
            if (XCAFDoc_ShapeTool::IsShape(childLabel))
            {
                TopoDS_Shape shape = XCAFDoc_ShapeTool::GetShape(childLabel);
                buildShapeTree(shape, childItem);
            }
        }
        else
        {
            buildAssemblyTree( tree, childId, childItem );
        }
        childId = tree.nodeSiblingNext( childId );
    }
}

void TreeWidget::buildShapeTree(const TopoDS_Shape& shape, const QTreeWidgetItem* parentItem)
{
#ifdef mydebug
    std::cout << " Shape Type: " << shape.ShapeType() << std::endl;
#endif
    // Shell
    for (TopExp_Explorer shellExp(shape, TopAbs_SHELL); shellExp.More(); shellExp.Next())
    {
        TopoDS_Shell shell = TopoDS::Shell(shellExp.Current());
#ifdef mydebug
        std::cout << "  Shell" << std::endl;
#endif
        // Face
        for (TopExp_Explorer faceExp(shell, TopAbs_FACE); faceExp.More(); faceExp.Next())
        {
            TopoDS_Face face = TopoDS::Face(faceExp.Current());
#ifdef mydebug
            std::cout << "    Face" << std::endl;
#endif
            //  Edge
            for (TopExp_Explorer edgeExp(face, TopAbs_EDGE); edgeExp.More(); edgeExp.Next())
            {
                TopoDS_Edge edge = TopoDS::Edge(edgeExp.Current());
#ifdef mydebug
                std::cout << "      Edge" << std::endl;
#endif
                //  Vertex
                    for (TopExp_Explorer vertexExp(edge, TopAbs_VERTEX); vertexExp.More(); vertexExp.Next())
                {
                    TopoDS_Vertex vertex = TopoDS::Vertex(vertexExp.Current());
                    gp_Pnt point = BRep_Tool::Pnt(vertex);
#ifdef mydebug
                    std::cout << "        Vertex: (" << point.X() << ", " << point.Y() << ", " << point.Z() << ")" << std::endl;
#endif
                }
            }
        }
    }
}

void TreeWidget::onItemClicked(QTreeWidgetItem* item, int column)
{
    if (item && m_modelTree) {
        bool ok;
        TreeNodeId nodeId = item->data(0, Qt::UserRole).toUInt(&ok);
        if (ok) {
            TDF_Label label = m_modelTree->nodeData(nodeId);
            emit labelSelected(label);
        }
    }
}
