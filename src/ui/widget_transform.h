#pragma once
#include <map>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>

#include <QWidget>

#include "display/ManipulatorObserver.h"

// Forward declaration 
//class TopoDS_Shape;
class AIS_InteractiveObject;
class QCloseEvent;

namespace Ui {
class WidgetTransform;
}
/*
 BUG : double click cancel pick manipulator?
*/
class WidgetTransform : public QWidget, public ManipulatorObserver
{
    Q_OBJECT

public:
    explicit WidgetTransform(QWidget *parent = nullptr);
    ~WidgetTransform() override;

    void show();
    void hide();

    // ManipulatorObserver interface
    void onManipulatorChange(const gp_Trsf& trsf) override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onPickClicked();
    void onObjectSelected(const TopoDS_Shape& shape);
    void onTransformChanged();
    void onResetClicked();
    void onCloseClicked();

    //void onManipulatorChanged(const gp_Trsf& trsf); // Removed, replaced by override

private:
    void saveMouseState();
    void restoreMouseState();
    void updateTransform();

private:
    Ui::WidgetTransform* ui;
    
    // State saving
    int m_savedMouseMode;
    std::map<TopAbs_ShapeEnum, bool> m_savedFilters;
    
    bool m_isPicking;
    Handle(AIS_InteractiveObject) m_targetObject;
    TopoDS_Shape m_targetShape;
};
