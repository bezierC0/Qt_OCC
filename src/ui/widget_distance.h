#pragma once
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

#include <QWidget>

// Forward declaration 
class QCloseEvent;
class SelectionPickSession;

namespace Ui {
class WidgetDistance;
}

class WidgetDistance : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetDistance(QWidget *parent = nullptr);
    ~WidgetDistance() override;

    void show();
    void hide();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onPickFirstClicked();
    void onPickSecondClicked();
    void onClearFirstClicked();
    void onClearSecondClicked();
    void onObjectSelected(const TopoDS_Shape& shape);
    void onCloseClicked();

private:
    enum PickingState {
        Idle,
        PickFirst,
        PickSecond
    };

    void startPicking(PickingState target);
    void restoreMouseState();
    void updateUI();
    void calculateDistance();

private:
    Ui::WidgetDistance* ui;
    
    SelectionPickSession* m_pickSession;
    
    PickingState m_pickingState;

    gp_Pnt m_pnt1;
    gp_Pnt m_pnt2;

    bool m_hasP1;
    bool m_hasP2;
};
