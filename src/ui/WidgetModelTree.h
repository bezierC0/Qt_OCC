#pragma once
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QHeaderView>
#include <memory>

class TreeWidget;
class TDF_Label;
class ModelTreeContextMenu;
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
    void labelPickRequested(const TDF_Label& label);
    void labelRemoveRequested(const TDF_Label& label);
private slots:
    void onContextMenuRequested(const QPoint& pos);
    void onContextMenuPick();
    void onContextMenuRemove();
private:
    TreeWidget*           m_treeWidget{nullptr};
    ModelTreeContextMenu* m_contextMenu{nullptr};
    std::unique_ptr<TDF_Label> m_pendingLabel;  // label under the last right-click

};