#pragma once
#include <map>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Trsf.hxx>
#include <TDF_Label.hxx>

#include <QPointer>
#include <QWidget>

#include "display/ManipulatorObserver.h"

// Forward declaration 
//class TopoDS_Shape;
class AIS_InteractiveObject;
class QCloseEvent;
class QKeyEvent;
class OCCView;

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
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onPickClicked();
    void onObjectSelected(const TopoDS_Shape& shape);
    void onTransformChanged();
    void onResetClicked();
    void onApplyClicked();
    void onCloseClicked();
    void onActiveViewChanged(OCCView* view);
    void onShapeObjectsChanged();
    void onCancelRequested();

    //void onManipulatorChanged(const gp_Trsf& trsf); // Removed, replaced by override

private:
    enum class TransformSource
    {
        Editors,
        Manipulator,
        Reset,
        Restore
    };

    void saveMouseState();
    void restoreMouseState();
    void finishInteraction();
    void restorePreview();
    void clearTarget();
    void updateTransform();
    void applyWorkingTransform(const gp_Trsf& transform, TransformSource source);
    void updateEditorsFromTransform(const gp_Trsf& transform);

private:
    Ui::WidgetTransform* ui;
    
    // State saving
    int m_savedMouseMode{0};
    std::map<TopAbs_ShapeEnum, bool> m_savedFilters;
    bool m_hasSavedMouseState{false};
    
    bool m_isPicking;
    QPointer<OCCView> m_view;
    Handle(AIS_InteractiveObject) m_targetObject;
    TopoDS_Shape m_targetShape;
    TDF_Label m_targetLabel;
    gp_Trsf m_initialTransform;
    gp_Trsf m_originalTransform;
    gp_Trsf m_workingTransform;
    bool m_hasOriginalTransform{false};
};
