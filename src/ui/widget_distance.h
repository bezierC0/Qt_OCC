#pragma once
#include <map>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

#include <QWidget>

// Forward declaration 
class QCloseEvent;

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
    void onPickClicked();
    void onObjectSelected(const TopoDS_Shape& shape);
    void onCloseClicked();

private:
    void saveMouseState();
    void restoreMouseState();
    void updateUI();
    void calculateDistance();

private:
    Ui::WidgetDistance* ui;
    
    // State saving
    int m_savedMouseMode;
    std::map<TopAbs_ShapeEnum, bool> m_savedFilters;
    
    enum PickingState {
        Idle,
        PickFirst,
        PickSecond
    };
    PickingState m_pickingState;

    gp_Pnt m_pnt1;
    gp_Pnt m_pnt2;

    bool m_hasP1;
    bool m_hasP2;
};
