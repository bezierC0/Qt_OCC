#pragma once
#include <map>
#include <vector>
#include <memory>

#include <Standard_WarningsDisable.hxx>
#include <QOpenGLWidget>
#include <Standard_WarningsRestore.hxx>

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewController.hxx>
#include <V3d_View.hxx>
#include <TopAbs_ShapeEnum.hxx>



class AIS_ViewCube;
class AIS_Manipulator;
class TopoDS_Face;
class TopoDS_Shape;

namespace View
{
enum MouseMode { NONE, SELECTION, END };
enum DisplayMode {
    MODE_SHADED = 0,
    MODE_WIREFRAME,
    MODE_HIDDEN_LINE,
    MODE_SHADED_WITH_EDGES,
    MODE_END
};
class WorkPlane{
public:
    Aspect_GridType m_gridType{Aspect_GridType::Aspect_GT_Rectangular};
    Aspect_GridDrawMode m_gridDrawMode{Aspect_GridDrawMode::Aspect_GDM_Lines};
};
class ClippingPlane{
public:
    ClippingPlane();
    explicit ClippingPlane(const Handle(Graphic3d_ClipPlane)& clipPlane, bool isOn = false,bool isCapping = false) ;
    Handle(Graphic3d_ClipPlane) m_clipPlane{nullptr};
};
class IInterferece{
public:
    virtual ~IInterferece() = default;
};
class InterfereceImpl
    : public IInterferece
{
public:
    Handle(AIS_InteractiveObject)       m_object          {nullptr};
    Handle(AIS_InteractiveObject)       m_boundingBox     {nullptr};
};

class InterfereceSetting{
public:
    InterfereceSetting( double r = 0.0, double g = 0.0, double b = 0.0, double a = 0.0, double width = 0.0, bool display = true )
        : m_colorR( r ),
          m_colorG( g ),
          m_colorB( b ),
          m_colorA( a ),
          m_width( width ),
          isDisplayBoundingBox( display )
    {
    }

    double m_colorR;
    double m_colorG;
    double m_colorB;
    double m_colorA;
    double m_width;
    bool isDisplayBoundingBox;
};

} // namespace View

//! OCCT 3D View.
class OCCView : public QOpenGLWidget, public AIS_ViewController
{
    Q_OBJECT
public:
    //! Main constructor.
    OCCView(QWidget *theParent = nullptr);

    //! Destructor.
    ~OCCView() override;

    //! Return Viewer.
    const Handle(V3d_Viewer) & Viewer() const { return m_viewer; }

    //! Return View.
    const Handle(V3d_View) & View() const { return m_view; }

    //! Return AIS context.
    const Handle(AIS_InteractiveContext) & Context() const { return m_context; }

    //! Return OpenGL info.
    const QString &getGlInfo() const { return myGlInfo; }

    //! Minial widget size.
    QSize minimumSizeHint() const override { return QSize(400, 400); }

    //! Default widget size.
    QSize sizeHint() const override { return QSize(720, 480); }

    //! Update selection filter status.
    void updateSelectionFilter(TopAbs_ShapeEnum target, bool theIsActive);

    void clearShape();

    void setShape(const Handle(AIS_InteractiveObject) & loadedShape);

    const std::vector<Handle(AIS_InteractiveObject)> &getShapeObjects() const;

    const std::vector<Handle(AIS_InteractiveObject)> &getSelectedObjects() const;

    std::vector<Handle(AIS_Shape)> getSelectedAisShape(int count) const;

    void clearSelectedObjects();

    void attachManipulator(const Handle(AIS_InteractiveObject) object);

    const std::map<TopAbs_ShapeEnum, bool> &getSelectionFilters() const;

    void transform(); 

    void checkInterference();

    void reDraw();

    void viewfit();

    void viewUpdate();

    /* view change */
    void viewIsometric();
    void viewTop();
    void viewBottom();
    void viewLeft();
    void viewRight();
    void viewFront();
    void viewBack();
    void setDisplayMode(View::DisplayMode mode);
    void createWorkPlane(double x, double y, double z, double dx, double dy, double dz);
    bool isWorkPlaneActive() const;
    void deactivateWorkPlane();

    void addClippingPlane(const gp_Pnt& point,const gp_Dir& normal, bool isOn = true, bool isCap = true) ;
    void setClippingPlaneIsOn(const bool isOn) ;
    void setCappingPlaneIsCap(const bool isCap) ;
    //void removeClippingPlane() ;

    void explosion(double theFactor) ;

    void setMouseMode(View::MouseMode mode);

    /* BRepOffsetAPI_MakeThickSolid */
    void makeHollow(const TopoDS_Shape shape, const TopoDS_Face& face);

    /* MakeOffsetShape */
    void makeThickness(const TopoDS_Shape shape, const Standard_Real thickness);

    void makeHole();

    void makeCutout();

    /* Pattern(linear) */
    void patternLinear(const TopoDS_Shape& baseShape,
        const gp_Vec& direction,
        Standard_Real spacing,
        Standard_Integer count);

    /* Pattern(Circular) */
    void patternCircular(
        const TopoDS_Shape& baseShape,
        const gp_Ax1& axis,
        Standard_Real angleStep,
        Standard_Integer count);

    /* mirrorByPlane */
    void mirrorByPlane(const TopoDS_Shape& shape,
        const gp_Pln& mirrorPlane);

    /* mirrorByAxis */
    void mirrorByAxis(const TopoDS_Shape& shape,
        const gp_Ax1& mirrorAxis);

    //! Handle subview focus change.
    void OnSubviewChanged(const Handle(AIS_InteractiveContext) &, const Handle(V3d_View) &,
                          const Handle(V3d_View) & theNewView) override;

protected:
    //! Initialize OpenGL.
    void initializeGL() override;

    //! Paint OpenGL.
    void paintGL() override;

protected:
    void closeEvent(QCloseEvent *theEvent) override;
    //! Handle mouse release event.
    void keyPressEvent(QKeyEvent *theEvent) override;
    //! Handle mouse move event.
    void mousePressEvent(QMouseEvent *theEvent) override;
    //! Handle wheel event.
    void mouseReleaseEvent(QMouseEvent *theEvent) override;
    //! Handle mouse move event.
    void mouseMoveEvent(QMouseEvent *theEvent) override;
    //! Handle mouse wheel event.
    void wheelEvent(QWheelEvent *theEvent) override;

private:
    //! Dump OpenGL info.
    void dumpGlInfo(bool theIsBasic, bool theToPrint);

    //! Animate camera transition from one state to another.
    void animateCamera(const Handle(Graphic3d_Camera) & theCamStart,
                       const Handle(Graphic3d_Camera) & theCamEnd);

    //! Animate view change to a specific orientation.
    void animateViewChange(V3d_TypeOfOrientation theOrientation);

    //! Request widget paintGL() event.
    void updateView();

    //! Handle view redraw.
    void handleViewRedraw(const Handle(AIS_InteractiveContext) & theCtx,
                          const Handle(V3d_View) & theView) override;
    //! Viewer handle.

    //! View handle.
private:
    Handle(V3d_Viewer)              m_viewer; //! AIS context handle.
    Handle(V3d_View)                m_view; //! View cube handle.
    Handle(AIS_InteractiveContext)  m_context;
    Handle(AIS_ViewCube)            m_navigationView; // XYZ Navigation GuiDocument::setViewTrihedronMode
    Handle(AIS_Manipulator)         m_manipulator;

    //! OpenGL info.
    Handle(V3d_View) myFocusView;
    //! Core profile flag.

    std::vector<Handle(AIS_InteractiveObject)> m_loadedObjects;
    std::vector<Handle(AIS_InteractiveObject)> m_selectedObjects;
    std::vector<std::shared_ptr<View::IInterferece>> m_interferenceObjects;

    QString myGlInfo;
    bool myIsCoreProfile;

    //! Active selection filters map.
    std::map<TopAbs_ShapeEnum, bool> m_selectionFilters{
        {TopAbs_VERTEX, false}, {TopAbs_EDGE, false}, {TopAbs_FACE, false}, {TopAbs_SOLID, true}};

    Standard_Real                                                       m_animationDuration{1}; // animation duration in seconds
    int                                                                 m_mouseMode{0};                   // 0 normal 1 select + normal
    View::DisplayMode                                                   m_displayMode{View::DisplayMode::MODE_SHADED};
    View::WorkPlane                                                     m_workPlane{};
    std::vector<std::shared_ptr<View::ClippingPlane>>                   m_clippingPlanes{};
    static View::InterfereceSetting                                     m_interfereceSetting ;
};
