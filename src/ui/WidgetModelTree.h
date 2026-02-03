#pragma once
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QHeaderView>

class TreeWidget;
class TDF_Label;
template<typename T>
class Tree;

class ModelTreeWidget : public QWidget {
    Q_OBJECT
public:
    explicit  ModelTreeWidget(QWidget* widget = nullptr) ;
    ~ModelTreeWidget() override;
    void setModelTree( const Tree<TDF_Label>& modelTree);
signals:
    void labelSelected(const TDF_Label& label);
private:
    TreeWidget* m_treeWidget = nullptr; // TreeWidget

};