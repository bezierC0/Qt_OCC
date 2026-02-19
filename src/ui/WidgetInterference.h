#pragma once
#include <memory>
namespace View { class SelectedEntity; }
#include <QWidget>
#include <vector>
#include <AIS_InteractiveObject.hxx>

class QListWidget;
class QTreeWidget;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QTreeWidgetItem;

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

private:
    void setupUi();
    void updateInputList();

private:
    QVBoxLayout* m_mainLayout{};
    QListWidget* m_inputListWidget{};
    QTreeWidget* m_resultTreeWidget{};
    QPushButton* m_btnAdd{};
    QPushButton* m_btnRemove{};
    QPushButton* m_btnCheck{};

    std::vector<std::shared_ptr<View::SelectedEntity>> m_inputObjects;
};
