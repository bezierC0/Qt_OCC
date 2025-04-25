#pragma once

#include <QWidget>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <TopoDS_Shape.hxx>

class OCCView;

class ViewerWidget : public QWidget {
    Q_OBJECT
public:
    ViewerWidget(QWidget* parent = nullptr);
    void loadModel(const QString& filename);
    void setTopView();
    void checkInterference();

private:
    Handle(V3d_View) m_view;
    Handle(AIS_InteractiveContext) m_context;
    OCCView* m_occView;
    TopoDS_Shape m_loadedShape;
};
