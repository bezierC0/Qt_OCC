#pragma once
#include <memory>
namespace View { class SelectedEntity; }
#include <QWidget>
#include <vector>
#include <AIS_InteractiveObject.hxx>
#include <TopoDS_Shape.hxx>

class QListWidget;
class QTreeWidget;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QTreeWidgetItem;
class QProgressBar;
class QCheckBox;

namespace Ui {
class WidgetInterference;
}

class WidgetInterference : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetInterference(QWidget *parent = nullptr);
    ~WidgetInterference() override;

    void show();

private slots:
    void onAddClicked();
    void onRemoveClicked();
    void onCheckClicked();
    void onAddAllClicked();
    void onResultClicked(QTreeWidgetItem* item, int column);

private:
    void setupUi();
    void updateInputList();
    void applyResultDisplayOptions();
    void cycleObjectStyle(QPushButton* button, int& styleIndex);

    struct ResultContext
    {
        TopoDS_Shape shape;
        Handle(AIS_InteractiveObject) objectA;
        Handle(AIS_InteractiveObject) objectB;
    };

private:
    QVBoxLayout* m_mainLayout{};
    QListWidget* m_inputListWidget{};
    QTreeWidget* m_resultTreeWidget{};
    QPushButton* m_btnAdd{};
    QPushButton* m_btnRemove{};
    QPushButton* m_btnCheck{};
    QPushButton* m_btnAddAll{};
    QProgressBar* m_progressBar{};
    QCheckBox* m_resultOnTop{};
    QPushButton* m_objectAStyle{};
    QPushButton* m_objectBStyle{};
    QCheckBox* m_showOnlyPair{};
    int m_objectAStyleIndex{0};
    int m_objectBStyleIndex{0};

    std::vector<std::shared_ptr<View::SelectedEntity>> m_inputObjects;
    std::vector<ResultContext> m_resultContexts;
};
