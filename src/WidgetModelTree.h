#pragma once
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QHeaderView>
#include "Tree.h"

class TreeWidget;
class ModelTreeWidget : public QWidget {
    Q_OBJECT
public:
    explicit  ModelTreeWidget(QWidget* widget = nullptr) ;
    ~ModelTreeWidget() override;
    void setModelTree( const Tree<TDF_Label>& modelTree);
private:
    TreeWidget* m_treeWidget = nullptr;
};