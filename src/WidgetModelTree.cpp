#include "WidgetModelTree.h"
#include "TreeWidget.h"


ModelTreeWidget::ModelTreeWidget(QWidget* widget)
    : QWidget(widget)
{
    m_treeWidget = new TreeWidget( this );
    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->addWidget( m_treeWidget );
    layout->setContentsMargins( 0, 0, 0, 0 );
    setLayout( layout );
}

ModelTreeWidget::~ModelTreeWidget()
= default;

void ModelTreeWidget::setModelTree( const Tree<TDF_Label> & modelTree )
{
    m_treeWidget->setData( modelTree );
}
