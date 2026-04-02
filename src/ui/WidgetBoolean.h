#pragma once
#include <memory>
namespace View { class SelectedEntity; }
#include <QWidget>
#include <vector>
#include <AIS_InteractiveObject.hxx>

class QListWidget;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QCheckBox;
class QComboBox;
class ViewerWidget;

class WidgetBoolean : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetBoolean(ViewerWidget *parent = nullptr);
    ~WidgetBoolean() override;

    void show();
    void setOperationType(int type);

private slots:
    void onAddClicked();
    void onRemoveClicked();
    void onApplyClicked();

private:
    void setupUi();
    void updateInputList();

private:
    QVBoxLayout* m_mainLayout{};
    QListWidget* m_inputListWidget{};
    QPushButton* m_btnAdd{};
    QPushButton* m_btnRemove{};
    QPushButton* m_btnApply{};
    QCheckBox* m_chkDeleteOriginal{};
    QComboBox* m_cmbOperationType{};

    std::vector<std::shared_ptr<View::SelectedEntity>> m_inputObjects;
    ViewerWidget* m_parentViewer{};
};
