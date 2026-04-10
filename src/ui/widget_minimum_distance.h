#pragma once
#include <map>
#include <vector>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

#include <QWidget>
#include <AIS_InteractiveObject.hxx>

// Forward declaration 
class QCloseEvent;

namespace Ui {
class WidgetMinimumDistance;
}

class WidgetMinimumDistance : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetMinimumDistance(QWidget *parent = nullptr);
    ~WidgetMinimumDistance() override;

    void show();
    void hide();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onPickClicked();
    void onApplyClicked();
    void onObjectSelected(const TopoDS_Shape& shape);
    void onCloseClicked();

private:
    void saveMouseState();
    void restoreMouseState();
    void updateUI();
    void calculateMinimumDistance();
    QString getShapeNameAndType(const TopoDS_Shape& shape);
    void clearResultDisplay();

private:
    Ui::WidgetMinimumDistance* ui;
    
    // State saving
    int m_savedMouseMode;
    std::map<TopAbs_ShapeEnum, bool> m_savedFilters;
    
    enum PickingState {
        Idle,
        PickFirst,
        PickSecond
    };
    PickingState m_pickingState;

    TopoDS_Shape m_shape1;
    TopoDS_Shape m_shape2;

    bool m_hasP1;
    bool m_hasP2;

    std::vector<Handle(AIS_InteractiveObject)> m_resultObjects;
};
