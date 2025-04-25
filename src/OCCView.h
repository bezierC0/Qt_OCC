#pragma once

#include <QWidget>
#include <QOpenGLWidget>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <WNT_Window.hxx>

class OCCView
  : public QOpenGLWidget {
    Q_OBJECT
public:
    OCCView(QWidget* parent = nullptr);
    Handle(V3d_View) getView() const;
    Handle(AIS_InteractiveContext) getContext() const;

protected:
    void initializeGL() override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    Handle(Graphic3d_GraphicDriver) m_graphicDriver;
    Handle(V3d_Viewer) m_viewer;
    Handle(V3d_View) m_view;
    Handle(AIS_InteractiveContext) m_context;
    Handle( WNT_Window ) m_wntWindow;
};
