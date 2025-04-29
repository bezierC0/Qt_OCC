#pragma once

#include <Standard_WarningsDisable.hxx>
#include <QOpenGLWidget>
#include <Standard_WarningsRestore.hxx>

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewController.hxx>
#include <V3d_View.hxx>

class AIS_ViewCube;

//! OCCT 3D View.
class OCCView : public QOpenGLWidget, public AIS_ViewController
{
  Q_OBJECT
public:

  //! Main constructor.
  OCCView (QWidget* theParent = nullptr);

  //! Destructor.
  ~OCCView() override;

  //! Return Viewer.
  const Handle(V3d_Viewer)& Viewer() const { return myViewer; }

  //! Return View.
  const Handle(V3d_View)& View() const { return myView; }

  //! Return AIS context.
  const Handle(AIS_InteractiveContext)& Context() const { return myContext; }

  //! Return OpenGL info.
  const QString& getGlInfo() const { return myGlInfo; }

  //! Minial widget size.
  QSize minimumSizeHint() const override { return QSize(400, 400); }

  //! Default widget size.
  QSize sizeHint()        const override { return QSize(720, 480); }

  void setShape( const Handle( AIS_Shape )& loadedShape );

  void reDraw()const ;

  void clipping() const;

  void explosion() const;

public:

  //! Handle subview focus change.
  void OnSubviewChanged(const Handle(AIS_InteractiveContext)&,
                        const Handle(V3d_View)&,
                        const Handle(V3d_View)& theNewView) override;

protected: // OpenGL events

  //! Initialize OpenGL.
  //! Initialize OpenGL.
  //! Paint OpenGL.
  void initializeGL() override;
  //! Paint OpenGL.
  void paintGL() override;
  //virtual void resizeGL(int , int ) override;

  //! Handle close event.
protected: // user input events
  //! Handle key press event.

  //! Handle mouse press event.
  void closeEvent       (QCloseEvent*  theEvent) override;
  //! Handle mouse release event.
  void keyPressEvent    (QKeyEvent*    theEvent) override;
  //! Handle mouse move event.
  void mousePressEvent  (QMouseEvent*  theEvent) override;
  //! Handle wheel event.
  void mouseReleaseEvent(QMouseEvent*  theEvent) override;
  void mouseMoveEvent   (QMouseEvent*  theEvent) override;
  void wheelEvent       (QWheelEvent*  theEvent) override;

private:

  //! Dump OpenGL info.
  void dumpGlInfo (bool theIsBasic, bool theToPrint);

  //! Request widget paintGL() event.
  void updateView();

  //! Handle view redraw.
  void handleViewRedraw (const Handle(AIS_InteractiveContext)& theCtx,
                                 const Handle(V3d_View)& theView) override;
  //! Viewer handle.

  //! View handle.
private:
  //! AIS context handle.
  Handle(V3d_Viewer)             myViewer;
  //! View cube handle.
  Handle(V3d_View)               myView;
  Handle(AIS_InteractiveContext) myContext;
  //! Focus view handle.
  Handle(AIS_ViewCube)           m_navigationView; // XYZ Navigation

  //! OpenGL info.
  Handle(V3d_View)               myFocusView;
  //! Core profile flag.

  Handle( AIS_Shape )           m_loadedShape;

  QString myGlInfo;
  bool myIsCoreProfile;
};
