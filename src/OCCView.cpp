#include "OCCView.h"
#include "CollisionDetector.h"
#include "OcctGlTools.h"
#include "TopoShapeUtil.h"
#include "SelectedEntity.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <OpenGl_Context.hxx>

#include <QApplication>
#include <QMessageBox>
#include <QMouseEvent>

#include <iostream>
#include <cmath>

#include <Standard_WarningsRestore.hxx>
#include <Standard_WarningsDisable.hxx>

#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>
#include <AIS_Manipulator.hxx>
#include <AIS_AnimationCamera.hxx>
#include <AIS_ConnectedInteractive.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <Graphic3d_Camera.hxx>
#include <gp_Trsf.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <TopoDS_Shape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <Message.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OpenGl_FrameBuffer.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFPrs_AISObject.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt2d.hxx>
#include <ProjLib.hxx>
#include <ElSLib.hxx>
#include <NCollection_DataMap.hxx>




View::InterfereceSetting OCCView::m_interfereceSetting{ 1.0, 0.0, 0.0, 0.0, 10, true };

namespace
{
    //! Map Qt buttons bitmask to virtual keys.
    Aspect_VKeyMouse qtMouseButtons2VKeys(Qt::MouseButtons theButtons)
    {
        Aspect_VKeyMouse aButtons = Aspect_VKeyMouse_NONE;
        if ((theButtons & Qt::LeftButton) != 0)
        {
            aButtons |= Aspect_VKeyMouse_LeftButton;
        }
        if ((theButtons & Qt::MiddleButton) != 0)
        {
            aButtons |= Aspect_VKeyMouse_MiddleButton;
        }
        if ((theButtons & Qt::RightButton) != 0)
        {
            aButtons |= Aspect_VKeyMouse_RightButton;
        }
        return aButtons;
    }

    //! Map Qt mouse modifiers bitmask to virtual keys.
    Aspect_VKeyFlags qtMouseModifiers2VKeys(Qt::KeyboardModifiers theModifiers)
    {
        Aspect_VKeyFlags aFlags = Aspect_VKeyFlags_NONE;
        if ((theModifiers & Qt::ShiftModifier) != 0)
        {
            aFlags |= Aspect_VKeyFlags_SHIFT;
        }
        if ((theModifiers & Qt::ControlModifier) != 0)
        {
            aFlags |= Aspect_VKeyFlags_CTRL;
        }
        if ((theModifiers & Qt::AltModifier) != 0)
        {
            aFlags |= Aspect_VKeyFlags_ALT;
        }
        return aFlags;
    }

    //! Map Qt key to virtual key.
    Aspect_VKey qtKey2VKey(int theKey)
    {
        switch (theKey)
        {
        case 1060: // ru
        case Qt::Key_A:
            return Aspect_VKey_A;
        case 1048: // ru
        case Qt::Key_B:
            return Aspect_VKey_B;
        case 1057: // ru
        case Qt::Key_C:
            return Aspect_VKey_C;
        case 1042: // ru
        case Qt::Key_D:
            return Aspect_VKey_D;
        case 1059: // ru
        case Qt::Key_E:
            return Aspect_VKey_E;
        case 1040: // ru
        case Qt::Key_F:
            return Aspect_VKey_F;
        case Qt::Key_G:
            return Aspect_VKey_G;
        case Qt::Key_H:
            return Aspect_VKey_H;
        case Qt::Key_I:
            return Aspect_VKey_I;
        case Qt::Key_J:
            return Aspect_VKey_J;
        case Qt::Key_K:
            return Aspect_VKey_K;
        case 1044: // ru
        case Qt::Key_L:
            return Aspect_VKey_L;
        case Qt::Key_M:
            return Aspect_VKey_M;
        case Qt::Key_N:
            return Aspect_VKey_N;
        case Qt::Key_O:
            return Aspect_VKey_O;
        case Qt::Key_P:
            return Aspect_VKey_P;
        case 1049: // ru
        case Qt::Key_Q:
            return Aspect_VKey_Q;
        case 1050: // ru
        case Qt::Key_R:
            return Aspect_VKey_R;
        case 1067: // ru
        case Qt::Key_S:
            return Aspect_VKey_S;
        case 1045: // ru
        case Qt::Key_T:
            return Aspect_VKey_T;
        case Qt::Key_U:
            return Aspect_VKey_U;
        case 1052: // ru
        case Qt::Key_V:
            return Aspect_VKey_V;
        case 1062: // ru
        case Qt::Key_W:
            return Aspect_VKey_W;
        case 1063: // ru
        case Qt::Key_X:
            return Aspect_VKey_X;
        case Qt::Key_Y:
            return Aspect_VKey_Y;
        case 1071: // ru
        case Qt::Key_Z:
            return Aspect_VKey_Z;
            //
        case Qt::Key_0:
            return Aspect_VKey_0;
        case Qt::Key_1:
            return Aspect_VKey_1;
        case Qt::Key_2:
            return Aspect_VKey_2;
        case Qt::Key_3:
            return Aspect_VKey_3;
        case Qt::Key_4:
            return Aspect_VKey_4;
        case Qt::Key_5:
            return Aspect_VKey_5;
        case Qt::Key_6:
            return Aspect_VKey_6;
        case Qt::Key_7:
            return Aspect_VKey_7;
        case Qt::Key_8:
            return Aspect_VKey_8;
        case Qt::Key_9:
            return Aspect_VKey_9;
            //
        case Qt::Key_F1:
            return Aspect_VKey_F1;
        case Qt::Key_F2:
            return Aspect_VKey_F2;
        case Qt::Key_F3:
            return Aspect_VKey_F3;
        case Qt::Key_F4:
            return Aspect_VKey_F4;
        case Qt::Key_F5:
            return Aspect_VKey_F5;
        case Qt::Key_F6:
            return Aspect_VKey_F6;
        case Qt::Key_F7:
            return Aspect_VKey_F7;
        case Qt::Key_F8:
            return Aspect_VKey_F8;
        case Qt::Key_F9:
            return Aspect_VKey_F9;
        case Qt::Key_F10:
            return Aspect_VKey_F10;
        case Qt::Key_F11:
            return Aspect_VKey_F11;
        case Qt::Key_F12:
            return Aspect_VKey_F12;
            //
        case Qt::Key_Up:
            return Aspect_VKey_Up;
        case Qt::Key_Left:
            return Aspect_VKey_Left;
        case Qt::Key_Right:
            return Aspect_VKey_Right;
        case Qt::Key_Down:
            return Aspect_VKey_Down;
        case Qt::Key_Plus:
            return Aspect_VKey_Plus;
        case Qt::Key_Minus:
            return Aspect_VKey_Minus;
        case Qt::Key_Equal:
            return Aspect_VKey_Equal;
        case Qt::Key_PageDown:
            return Aspect_VKey_PageDown;
        case Qt::Key_PageUp:
            return Aspect_VKey_PageUp;
        case Qt::Key_Home:
            return Aspect_VKey_Home;
        case Qt::Key_End:
            return Aspect_VKey_End;
        case Qt::Key_Escape:
            return Aspect_VKey_Escape;
        case Qt::Key_Back:
            return Aspect_VKey_Back;
        case Qt::Key_Enter:
            return Aspect_VKey_Enter;
        case Qt::Key_Backspace:
            return Aspect_VKey_Backspace;
        case Qt::Key_Space:
            return Aspect_VKey_Space;
        case Qt::Key_Delete:
            return Aspect_VKey_Delete;
        case Qt::Key_Tab:
            return Aspect_VKey_Tab;
        case 1025:
        case Qt::Key_QuoteLeft:
            return Aspect_VKey_Tilde;
            //
        case Qt::Key_Shift:
            return Aspect_VKey_Shift;
        case Qt::Key_Control:
            return Aspect_VKey_Control;
        case Qt::Key_Alt:
            return Aspect_VKey_Alt;
        case Qt::Key_Menu:
            return Aspect_VKey_Menu;
        case Qt::Key_Meta:
            return Aspect_VKey_Meta;
        default:
            return Aspect_VKey_UNKNOWN;
        }
    }

}

namespace View
{
ClippingPlane::ClippingPlane() = default;
ClippingPlane::ClippingPlane(const Handle(Graphic3d_ClipPlane)& clipPlane, const bool isOn,const bool isCapping)
    : m_clipPlane(clipPlane)
{
    m_clipPlane->SetOn(isOn);
    m_clipPlane->SetCapping(isCapping);
}
} 


//! OpenGL FBO subclass for wrapping FBO created by Qt using GL_RGBA8 texture format instead of GL_SRGB8_ALPHA8.
//! This FBO is set to OpenGl_Context::SetDefaultFrameBuffer() as a final target.
//! Subclass calls OpenGl_Context::SetFrameBufferSRGB() with sRGB=false flag,
//! which asks OCCT to disable GL_FRAMEBUFFER_SRGB and apply sRGB gamma correction manually.
class OcctQtFrameBuffer : public OpenGl_FrameBuffer
{
    DEFINE_STANDARD_RTTI_INLINE(OcctQtFrameBuffer, OpenGl_FrameBuffer)
public:
    //! Empty constructor.
    OcctQtFrameBuffer() = default;

    //! Make this FBO active in context.
    void BindBuffer(const Handle(OpenGl_Context) & theGlCtx) override
    {
        OpenGl_FrameBuffer::BindBuffer(theGlCtx);
        theGlCtx->SetFrameBufferSRGB(true, false);
    }

    //! Make this FBO as drawing target in context.
    void BindDrawBuffer(const Handle(OpenGl_Context) & theGlCtx) override
    {
        OpenGl_FrameBuffer::BindDrawBuffer(theGlCtx);
        theGlCtx->SetFrameBufferSRGB(true, false);
    }

    //! Make this FBO as reading source in context.
    void BindReadBuffer(const Handle(OpenGl_Context) & theGlCtx) override
    {
        OpenGl_FrameBuffer::BindReadBuffer(theGlCtx);
    }
};

OCCView::OCCView(QWidget *theParent)
    : QOpenGLWidget(theParent),
      myIsCoreProfile(true)
{
    Handle(Aspect_DisplayConnection) aDisp = new Aspect_DisplayConnection();
    Handle(OpenGl_GraphicDriver) aDriver = new OpenGl_GraphicDriver(aDisp, false);
    // lets QOpenGLWidget to manage buffer swap
    aDriver->ChangeOptions().buffersNoSwap = true;
    // don't write into alpha channel
    aDriver->ChangeOptions().buffersOpaqueAlpha = true;
    // offscreen FBOs should be always used
    aDriver->ChangeOptions().useSystemBuffer = false;

    // create viewer
    m_viewer = new V3d_Viewer(aDriver);
    m_viewer->SetDefaultBackgroundColor(Quantity_NOC_CADETBLUE);
    m_viewer->SetDefaultLights();
    m_viewer->SetLightOn();
    //m_viewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines); // show grid grand

    // create AIS context
    m_context = new AIS_InteractiveContext(m_viewer);
    myViewAnimation = new AIS_AnimationCamera("fit", m_view);

    m_navigationView = new AIS_ViewCube();
    m_navigationView->SetViewAnimation(myViewAnimation);
    m_navigationView->SetFixedAnimationLoop(false);
    m_navigationView->SetAutoStartAnimation(true);
    m_navigationView->TransformPersistence()->SetOffset2d(Graphic3d_Vec2i(100, 150));

    // note - window will be created later within initializeGL() callback!
    m_view = m_viewer->CreateView();
    m_view->SetImmediateUpdate(false);
#ifndef __APPLE__
    m_view->ChangeRenderingParams().NbMsaaSamples = 4; // warning - affects performance
#endif
    m_view->ChangeRenderingParams().ToShowStats = true;
    m_view->ChangeRenderingParams().CollectedStats = (Graphic3d_RenderingParams::PerfCounters)(Graphic3d_RenderingParams::PerfCounters_FrameRate | Graphic3d_RenderingParams::PerfCounters_Triangles);

    myViewAnimation->SetOwnDuration(m_animationDuration);
    myViewAnimation->SetView(m_view);

    // Manipulator setup
    m_manipulator = new AIS_Manipulator();

    // Coordinate display setup
    if( m_showMouseCoordinates ){
        m_mouseCoordinateLabel = new AIS_TextLabel();
        m_mouseCoordinateLabel->SetColor(Quantity_NOC_WHITE);
        m_mouseCoordinateLabel->SetDisplayMode(AIS_WireFrame);
        m_mouseCoordinateLabel->SetTransformPersistence(new Graphic3d_TransformPers(Graphic3d_TMF_ZoomRotatePers)); // not zoom and rotate
        m_mouseCoordinateLabel->SetZLayer(Graphic3d_ZLayerId_Topmost);
        m_mouseCoordinateLabel->SetInfiniteState(true);// not fit?
        m_context->Display(m_mouseCoordinateLabel, false);
    }


    // Qt widget setup
    setMouseTracking(true);
    setBackgroundRole(QPalette::NoRole); // or NoBackground
    setFocusPolicy(Qt::StrongFocus);     // set focus policy to threat QContextMenuEvent from keyboard
    setUpdatesEnabled(true);
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

    // OpenGL setup managed by Qt
    QSurfaceFormat aGlFormat;
    aGlFormat.setDepthBufferSize(24);
    aGlFormat.setStencilBufferSize(8);
    // aGlFormat.setOption (QSurfaceFormat::DebugContext, true);
    aDriver->ChangeOptions().contextDebug = aGlFormat.testOption(QSurfaceFormat::DebugContext);
    // aGlFormat.setOption (QSurfaceFormat::DeprecatedFunctions, true);
    if (myIsCoreProfile)
    {
        aGlFormat.setVersion(4, 5);
    }
    aGlFormat.setProfile(myIsCoreProfile ? QSurfaceFormat::CoreProfile : QSurfaceFormat::CompatibilityProfile);

    setFormat(aGlFormat);
}

OCCView::~OCCView()
{
    // hold on X11 display connection till making another connection active by glXMakeCurrent()
    // to workaround sudden crash in QOpenGLWidget destructor
    Handle(Aspect_DisplayConnection) aDisp = m_viewer->Driver()->GetDisplayConnection();

    // release OCCT viewer
    m_context->RemoveAll(false);
    m_context.Nullify();
    m_view->Remove();
    m_view.Nullify();
    m_viewer.Nullify();

    // make active OpenGL context created by Qt
    makeCurrent();
    aDisp.Nullify();
}

void OCCView::updateSelectionFilter(TopAbs_ShapeEnum target, bool theIsActive)
{
    /*
    * graphics_scene.cpp
    * GraphicsScene::activateObjectSelection
    */
    m_selectionFilters.at(target) = theIsActive;
    for (const auto &filter : m_selectionFilters) {
        if (filter.second) {
            m_context->Activate(AIS_Shape::SelectionMode(filter.first));
        }else{
            m_context->Deactivate(AIS_Shape::SelectionMode(filter.first));
        }
    }
}

void OCCView::dumpGlInfo(bool theIsBasic, bool theToPrint)
{
    TColStd_IndexedDataMapOfStringString aGlCapsDict;
    m_view->DiagnosticInformation(aGlCapsDict, theIsBasic ? Graphic3d_DiagnosticInfo_Basic : Graphic3d_DiagnosticInfo_Complete);
    TCollection_AsciiString anInfo;
    for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter(aGlCapsDict); aValueIter.More(); aValueIter.Next())
    {
        if (!aValueIter.Value().IsEmpty())
        {
            if (!anInfo.IsEmpty())
            {
                anInfo += "\n";
            }
            anInfo += aValueIter.Key() + ": " + aValueIter.Value();
        }
    }

    if (theToPrint)
    {
        Message::SendInfo(anInfo);
    }
    myGlInfo = QString::fromUtf8(anInfo.ToCString());
}

// ================================================================
// Function : initializeGL
// Purpose  :
// ================================================================
void OCCView::initializeGL()
{
    const QRect aRect = rect();
    const Graphic3d_Vec2i aViewSize(aRect.right() - aRect.left(), aRect.bottom() - aRect.top());

    Aspect_Drawable aNativeWin = reinterpret_cast<Aspect_Drawable>(winId());
#ifdef _WIN32
    HDC aWglDevCtx = wglGetCurrentDC();
    HWND aWglWin = WindowFromDC(aWglDevCtx);
    aNativeWin = static_cast<Aspect_Drawable>(aWglWin);
#endif

    Handle(OpenGl_Context) aGlCtx = new OpenGl_Context();
    if (!aGlCtx->Init(myIsCoreProfile))
    {
        Message::SendFail() << "Error: OpenGl_Context is unable to wrap OpenGL context";
        QMessageBox::critical(0, "Failure", "OpenGl_Context is unable to wrap OpenGL context");
        QApplication::exit(1);
        return;
    }

    Handle(Aspect_NeutralWindow) aWindow = Handle(Aspect_NeutralWindow)::DownCast(m_view->Window());
    if (!aWindow.IsNull())
    {
        aWindow->SetNativeHandle(aNativeWin);
        aWindow->SetSize(aViewSize.x(), aViewSize.y());
        m_view->SetWindow(aWindow, aGlCtx->RenderingContext());
        // dumpGlInfo (true, true);
    }
    else
    {
        aWindow = new Aspect_NeutralWindow();
        aWindow->SetVirtual(true);
        aWindow->SetNativeHandle(aNativeWin);
        aWindow->SetSize(aViewSize.x(), aViewSize.y());
        m_view->SetWindow(aWindow, aGlCtx->RenderingContext());
        // dumpGlInfo (true, true);

        m_context->Display(m_navigationView, 0, 0, false);
    }
}

void OCCView::closeEvent(QCloseEvent *theEvent)
{
    theEvent->accept();
}

void OCCView::keyPressEvent(QKeyEvent *theEvent)
{
    Aspect_VKey aKey = qtKey2VKey(theEvent->key());
    switch (aKey)
    {
    case Aspect_VKey_Escape:
    {
        QApplication::exit();
        return;
    }
    case Aspect_VKey_F:
    {
        m_view->FitAll(0.01, false);
        update();
        return;
    }
    default:;
    }
    QOpenGLWidget::keyPressEvent(theEvent);
}

void OCCView::mousePressEvent(QMouseEvent *theEvent)
{
    if (m_mouseMode == View::MouseMode::SELECTION)
    {
        // Perform selection first before checking selected objects
        const AIS_SelectionScheme aScheme = (theEvent->modifiers() & Qt::ShiftModifier) ? AIS_SelectionScheme_Add : AIS_SelectionScheme_Replace;
        m_context->SelectDetected(aScheme);

        // Now process the selected objects
        if (m_context->NbSelected())
        {
            for (m_context->InitSelected(); m_context->MoreSelected(); m_context->NextSelected())
            {
                Handle(SelectMgr_EntityOwner) owner = m_context->SelectedOwner();
                if (!owner.IsNull())
                {
                    Handle(StdSelect_BRepOwner) brepOwner =  Handle(StdSelect_BRepOwner)::DownCast(owner);
                    if (!brepOwner.IsNull())
                    {
                        TopoDS_Shape selectedShape = brepOwner->Shape();
                        auto it = std::find_if (m_selectedObjects.begin(),
                                          m_selectedObjects.end(),
                                          [&](const auto& it){
                                            return selectedShape.IsSame(it->GetSelectedShape());
                                          });
                        if (it == m_selectedObjects.end())
                        {
                            Handle(AIS_InteractiveObject) parentObject = Handle(AIS_InteractiveObject)::DownCast(owner->Selectable());
                            auto entity = std::make_shared<View::SelectedEntity>(parentObject,selectedShape);
                            m_selectedObjects.emplace_back(entity);
                        }
                    }
                }

            }
        }
    }
    else
    {
        if (!m_selectedObjects.empty())
        {
            m_context->ClearSelected(false);
            m_selectedObjects.clear();
        }
    }

    const Graphic3d_Vec2i aPnt(theEvent->pos().x(), theEvent->pos().y());
    QOpenGLWidget::mousePressEvent(theEvent);
    const Aspect_VKeyFlags aFlags = qtMouseModifiers2VKeys(theEvent->modifiers());
    if (!m_view.IsNull() && UpdateMouseButtons(aPnt, qtMouseButtons2VKeys(theEvent->buttons()), aFlags, false))
    {
        updateView();
    }
}

void OCCView::mouseReleaseEvent(QMouseEvent *theEvent)
{
    QOpenGLWidget::mouseReleaseEvent(theEvent);
    const Graphic3d_Vec2i aPnt(theEvent->pos().x(), theEvent->pos().y());
    const Aspect_VKeyFlags aFlags = qtMouseModifiers2VKeys(theEvent->modifiers());
    if (!m_view.IsNull() && UpdateMouseButtons(aPnt,
                                               qtMouseButtons2VKeys(theEvent->buttons()),
                                               aFlags,
                                               false))
    {
        updateView();
    }
}

void OCCView::mouseMoveEvent(QMouseEvent *theEvent)
{
    QOpenGLWidget::mouseMoveEvent(theEvent);
    const Graphic3d_Vec2i aNewPos(theEvent->pos().x(), theEvent->pos().y());
    if (!m_view.IsNull() && UpdateMousePosition(aNewPos,
                                                qtMouseButtons2VKeys(theEvent->buttons()),
                                                qtMouseModifiers2VKeys(theEvent->modifiers()),
                                                false))
    {
        updateView();
    }

    // Update coordinate display at mouse position
    if( m_showMouseCoordinates )
    {
        updateCoordinateDisplay(theEvent->pos().x(), theEvent->pos().y());
    }
}

void OCCView::wheelEvent(QWheelEvent *theEvent)
{
    QOpenGLWidget::wheelEvent(theEvent);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    const Graphic3d_Vec2i aPos(Graphic3d_Vec2d(theEvent->position().x(), theEvent->position().y()));
#else
    const Graphic3d_Vec2i aPos(theEvent->pos().x(), theEvent->pos().y());
#endif
    if (m_view.IsNull())
    {
        return;
    }

    if (!m_view->Subviews().IsEmpty())
    {
        Handle(V3d_View) aPickedView = m_view->PickSubview(aPos);
        if (!aPickedView.IsNull() && aPickedView != myFocusView)
        {
            // switch input focus to another subview
            OnSubviewChanged(m_context, myFocusView, aPickedView);
            updateView();
            return;
        }
    }

    if (UpdateZoom(Aspect_ScrollDelta(aPos, static_cast<double>(theEvent->angleDelta().y()) / 8.0)))
    {
        updateView();
    }
}

gp_Pnt OCCView::screenToWorld(double x, double y)
{
    if (m_view.IsNull())
    {
        return gp_Pnt(0, 0, 0);
    }

    // Convert screen coordinates to 3D world coordinates directly
    double px = 0.0, py = 0.0, pz = 0.0;
    const int ix = std::lround(x);
    const int iy = std::lround(y);
    m_view->Convert(ix, iy, px, py, pz);

    return gp_Pnt(px, py, pz);
}

void OCCView::updateCoordinateDisplay(double screenX, double screenY)
{
    if (!m_showMouseCoordinates || m_mouseCoordinateLabel.IsNull() || m_view.IsNull())
    {
        return;
    }

    if(AIS_MouseGesture::AIS_MouseGesture_NONE != myMouseActiveGesture && m_context->IsDisplayed(m_mouseCoordinateLabel))
    {
        m_context->Erase(m_mouseCoordinateLabel, false);
        return;
    }

    // Update the stored world position
    m_currentMouseWorldPos = screenToWorld(screenX, screenY);

    // Format the coordinate text
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(0);
    ss << "X: " << m_currentMouseWorldPos.X()
        << ", Y: " << m_currentMouseWorldPos.Y()
        << ", Z: " << m_currentMouseWorldPos.Z();

    TCollection_AsciiString coordText(ss.str().c_str());

    m_mouseCoordinateLabel->SetText(coordText);

    // Position the text near the mouse cursor (offset to the right and slightly up)
    gp_Pnt textPos = screenToWorld(screenX + m_textOffsetX, screenY + m_textOffsetY);
    m_mouseCoordinateLabel->SetPosition(textPos);

    // Update the display
    if (!m_context->IsDisplayed(m_mouseCoordinateLabel))
    {
        m_context->Display(m_mouseCoordinateLabel, false);
    }
    m_context->Redisplay(m_mouseCoordinateLabel, false);
}

void OCCView::updateView()
{
    update();
    if (window() != NULL) { window()->update(); }
}

void OCCView::paintGL()
{
    if (m_view->Window().IsNull())
    {
        return;
    }
#ifdef _WIN32
    HDC aWglDevCtx = wglGetCurrentDC();
    HWND aWglWin = WindowFromDC(aWglDevCtx);
    auto aNativeWin = static_cast<Aspect_Drawable>(aWglWin);
#else
    auto aNativeWin = reinterpret_cast<Aspect_Drawable>(winId());
#endif

    if (m_view->Window()->NativeHandle() != aNativeWin)
    {
        // workaround window recreation done by Qt on monitor (QScreen) disconnection
        Message::SendWarning() << "Native window handle has changed by QOpenGLWidget!";
        initializeGL();
        return;
    }

    // wrap FBO created by QOpenGLWidget
    // get context from this (composer) view rather than from arbitrary one
    // Handle(OpenGl_GraphicDriver) aDriver = Handle(OpenGl_GraphicDriver)::DownCast (myContext->CurrentViewer()->Driver());
    // Handle(OpenGl_Context) aGlCtx = aDriver->GetSharedContext();
    Handle(OpenGl_Context) aGlCtx = OcctGlTools::GetGlContext(m_view);
    Handle(OpenGl_FrameBuffer) aDefaultFbo = aGlCtx->DefaultFrameBuffer();
    if (aDefaultFbo.IsNull())
    {
        aDefaultFbo = new OcctQtFrameBuffer();
        aGlCtx->SetDefaultFrameBuffer(aDefaultFbo);
    }
    if (!aDefaultFbo->InitWrapper(aGlCtx))
    {
        aDefaultFbo.Nullify();
        Message::DefaultMessenger()->Send("Default FBO wrapper creation failed", Message_Fail);
        QMessageBox::critical(nullptr, "Failure", "Default FBO wrapper creation failed");
        QApplication::exit(1);
        return;
    }

    Graphic3d_Vec2i aViewSizeOld;
    // const QRect aRect = rect(); Graphic3d_Vec2i aViewSizeNew(aRect.right() - aRect.left(), aRect.bottom() - aRect.top());
    Graphic3d_Vec2i aViewSizeNew = aDefaultFbo->GetVPSize();
    Handle(Aspect_NeutralWindow) aWindow = Handle(Aspect_NeutralWindow)::DownCast(m_view->Window());
    aWindow->Size(aViewSizeOld.x(), aViewSizeOld.y());
    if (aViewSizeNew != aViewSizeOld)
    {
        aWindow->SetSize(aViewSizeNew.x(), aViewSizeNew.y());
        m_view->MustBeResized();
        m_view->Invalidate();
        // dumpGlInfo (true, false);

        for (const Handle(V3d_View) &aSubviewIter : m_view->Subviews())
        {
            aSubviewIter->MustBeResized();
            aSubviewIter->Invalidate();
            aDefaultFbo->SetupViewport(aGlCtx);
        }
    }

    // flush pending input events and redraw the viewer
    Handle(V3d_View) aView = !myFocusView.IsNull() ? myFocusView : m_view;
    aView->InvalidateImmediate();
    FlushViewEvents(m_context, aView, true);
}

void OCCView::handleViewRedraw(const Handle(AIS_InteractiveContext) & theCtx,
                               const Handle(V3d_View) & theView)
{
    AIS_ViewController::handleViewRedraw(theCtx, theView);
    if (myToAskNextFrame)
    {
        // ask more frames for animation
        updateView();
    }
}

void OCCView::clearShape()
{
    for (const auto &object : m_loadedObjects)
    {
        m_context->Erase(object, false);
    }
    m_loadedObjects.clear();

    // clear interference
    for (const auto &object : m_interferenceObjects)
    {
        if (const auto objectImpl = std::reinterpret_pointer_cast<View::InterfereceImpl>(object))
        {
            m_context->Erase(objectImpl->m_object, false);
            m_context->Erase(objectImpl->m_boundingBox, false);
        }
    }
    m_interferenceObjects.clear();
}

void OCCView::setShape(const Handle(AIS_InteractiveObject) & loadedShape)
{
    m_loadedObjects.emplace_back(loadedShape);
}

void OCCView::removeShape(const TopoDS_Shape& removeShape)
{
    auto it = std::find_if(m_loadedObjects.begin(), m_loadedObjects.end(), [&](const Handle(AIS_InteractiveObject)& obj) {
        if (obj.IsNull()) {
            return false;
        }

        TopoDS_Shape currentShape;
        if (obj->IsKind(STANDARD_TYPE(AIS_Shape)))
        {
            currentShape = Handle(AIS_Shape)::DownCast(obj)->Shape();
        }
        else if (obj->IsKind(STANDARD_TYPE(XCAFPrs_AISObject)))
        {
            return false; 
        }

        return !currentShape.IsNull() && currentShape.IsSame(removeShape);
    });

    if (it != m_loadedObjects.end()) {
        // remove from context (preview)
        m_context->Erase(*it, false);
        
        m_loadedObjects.erase(it);
    }
    auto size1 = m_loadedObjects.size();
}
const std::vector<Handle(AIS_InteractiveObject)> &OCCView::getShapeObjects() const
{
    return m_loadedObjects;
}

const std::vector<std::shared_ptr<View::SelectedEntity>> &OCCView::getSelectedObjects() const
{
    return m_selectedObjects;
}

void OCCView::clearSelectedObjects()
{
    if (!m_selectedObjects.empty())
    {
        m_context->ClearSelected(false);
    }
    m_selectedObjects.clear();
}

std::vector<Handle( AIS_Shape )> OCCView::getSelectedAisShape( const int count ) const
{
    std::vector<Handle( AIS_Shape )> selectedAisShapes;
    if ( m_selectedObjects.size() != count )
        return selectedAisShapes;
    for( int i = 0; i < count; ++i ){
        const auto obj = m_selectedObjects.at(i)->m_parentObject;
        if ( obj.IsNull() )
            return selectedAisShapes;

        const auto aisShape = Handle(AIS_Shape)::DownCast(obj);
        if (aisShape.IsNull() )
            return selectedAisShapes;
        selectedAisShapes.emplace_back(aisShape);
    }
    return selectedAisShapes;
}

void OCCView::attachManipulator(const Handle(AIS_InteractiveObject) object)
{
    if( !m_manipulator )
        return ;
    
    m_manipulator->Attach(object);
    const auto aisShape = Handle(AIS_Shape)::DownCast(object);
    if( !aisShape || aisShape.IsNull() )
        return ;
    TopoDS_Shape shape = aisShape->Shape();
    Bnd_Box boundingBox;
    BRepBndLib::Add(shape, boundingBox);
    if (boundingBox.IsVoid()) {
        return;
    }
    Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
    boundingBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    const gp_Pnt point((xMin + xMax) / 2.0, 
                       (yMin + yMax) / 2.0, 
                       (zMin + zMax) / 2.0);

    const gp_Dir dir(0.0,0.0,1.0);
    const gp_Ax2 theA2(point,dir);
    m_manipulator->SetPosition(theA2);
}

const std::map<TopAbs_ShapeEnum, bool>& OCCView::getSelectionFilters() const
{
    return m_selectionFilters;
}

void OCCView::transform()
{
    if (!m_loadedObjects.empty())
    {
        Handle(AIS_InteractiveObject) obj = m_loadedObjects.at(0);
        auto aisShape = Handle(AIS_Shape)::DownCast(obj);
        if ( !aisShape || aisShape.IsNull())
        {
            return;
        }

        attachManipulator(obj);

        #if 0
        gp_Trsf currentTrsf = aisShape->LocalTransformation();


        gp_Trsf translation;
        translation.SetTranslation(gp_Vec(100.0, 0.0, 0.0));
        currentTrsf.Multiply(translation);

        aisShape->SetLocalTransformation(currentTrsf);

        Context()->Redisplay(aisShape, true);
        #endif
    }
    reDraw();
}

void OCCView::checkInterference()
{
    auto getShape = [](const Handle(AIS_InteractiveObject) & object) -> TopoDS_Shape
    {
        if (object.IsNull())
        {
            return {};
        }
        if (object->IsKind(STANDARD_TYPE(AIS_Shape)))
        {
            return Handle(AIS_Shape)::DownCast(object)->Shape();
        }
        if (object->IsKind(STANDARD_TYPE(XCAFPrs_AISObject)))
        {
            auto xcafObj = Handle(XCAFPrs_AISObject)::DownCast(object);
            return {}; // Returning empty shape for now.
        }
        return {};
    };

    CollisionDetector collsion { m_context };
    const auto &selectObjects = getSelectedObjects();

    std::vector<Handle(AIS_InteractiveObject)> results;
    for (size_t i = 0; i < selectObjects.size(); ++i)
    {
        for (size_t j = i + 1; j < selectObjects.size(); ++j)
        {
            Handle(AIS_InteractiveObject) objA = selectObjects.at(i)->GetParentInteractiveObject();
            Handle(AIS_InteractiveObject) objB = selectObjects.at(j)->GetParentInteractiveObject();

            const auto shapeA = getShape(objA);
            const auto shapeB = getShape(objB); // Fixed bug: was using objA for both

            if (shapeA.IsNull() || shapeB.IsNull())
                continue;

            if (!collsion.DetectAndHighlightCollision(shapeA, shapeB))
                continue;

            const auto &result = collsion.GetResult();
            if (result.IsNull())
            {
                continue;
            }

            objA->SetTransparency(0.1);
            objB->SetTransparency(0.1);
            
            results.emplace_back(result);
        }
    }
    auto createThickWireframe =[](const TopoDS_Shape& shape, 
                                                const Quantity_Color& color, 
                                                Standard_Real width) -> Handle(AIS_Shape)
    {
        Handle(AIS_Shape) wireframe = new AIS_Shape(shape);
        wireframe->SetDisplayMode(AIS_WireFrame);
        wireframe->SetColor(color);

        Handle(Prs3d_Drawer) drawer = wireframe->Attributes();
        
        Handle(Prs3d_LineAspect) lineAspect = new Prs3d_LineAspect(color, Aspect_TOL_SOLID, width);
        
        drawer->SetWireAspect(lineAspect);
        drawer->SetLineAspect(lineAspect);
        drawer->SetFreeBoundaryAspect(lineAspect);
        drawer->SetUnFreeBoundaryAspect(lineAspect);
        
        Handle(Prs3d_LineAspect) edgeAspect = new Prs3d_LineAspect(color, Aspect_TOL_SOLID, width);
        drawer->SetFaceBoundaryAspect(edgeAspect);
        
        drawer->SetFaceBoundaryDraw(Standard_True);
        
        return wireframe;
    };

    for (const auto &result : results)
    {
        std::shared_ptr<View::InterfereceImpl> resultImpl = std::make_shared<View::InterfereceImpl>();
        resultImpl->m_object = result;

        resultImpl->m_boundingBox = new AIS_Shape(TopoShape::Util::CreateBoundingBox(Handle(AIS_Shape)::DownCast(result)->Shape()));

        resultImpl->m_boundingBox->SetDisplayMode(AIS_WireFrame);
        resultImpl->m_boundingBox->SetColor(Quantity_Color(m_interfereceSetting.m_colorR, m_interfereceSetting.m_colorG, m_interfereceSetting.m_colorB, Quantity_TOC_RGB));
        // TODO BUG: width is not working
        resultImpl->m_boundingBox->SetWidth(m_interfereceSetting.m_width);
        //auto width = resultImpl->m_boundingBox->Width();
        // Handle(Prs3d_Drawer) drawer = resultImpl->m_boundingBox->Attributes();
        // Handle(Prs3d_LineAspect) lineAspect =
        //     new Prs3d_LineAspect(Quantity_Color(m_interfereceSetting.m_colorR, m_interfereceSetting.m_colorG, m_interfereceSetting.m_colorB, Quantity_TOC_RGB),
        //                          Aspect_TOL_SOLID,
        //                          m_interfereceSetting.m_width);
        // drawer->SetWireAspect(lineAspect);

        // resultImpl->m_boundingBox = createThickWireframe(
        //     TopoShape::Util::CreateBoundingBox(Handle(AIS_Shape)::DownCast(result)->Shape()),
        //     Quantity_Color(m_interfereceSetting.m_colorR, m_interfereceSetting.m_colorG, m_interfereceSetting.m_colorB, Quantity_TOC_RGB),
        //     m_interfereceSetting.m_width
        // );
        m_interferenceObjects.emplace_back(resultImpl);
    }

    reDraw();
}

void OCCView::reDraw()
{
    auto reDisplayMode = [&](const Handle(AIS_InteractiveObject) & object) {

        auto fnSetViewComputedMode = [=](bool on) {
            for (auto it = m_context->CurrentViewer()->DefinedViewIterator(); it.More(); it.Next())
                it.Value()->SetComputedMode(on);
        };
        if(m_displayMode == View::DisplayMode::MODE_HIDDEN_LINE){
            m_context->DefaultDrawer()->SetTypeOfHLR(Prs3d_TOH_PolyAlgo);
            m_context->DefaultDrawer()->EnableDrawHiddenLine();
            fnSetViewComputedMode(true);
        }
        else{
            m_context->DefaultDrawer()->SetTypeOfHLR(Prs3d_TOH_NotSet);
            m_context->DefaultDrawer()->DisableDrawHiddenLine();
            fnSetViewComputedMode(false);

            AIS_DisplayMode aisDispMode = AIS_Shaded;
            if (m_displayMode == View::DisplayMode::MODE_WIREFRAME) {
                aisDispMode = AIS_WireFrame;
            } else if (m_displayMode == View::DisplayMode::MODE_SHADED) {
                aisDispMode = AIS_Shaded;
            }
            if (aisDispMode != object->DisplayMode() )
                m_context->SetDisplayMode(object, aisDispMode, false);
            const bool showFaceBounds = m_displayMode == View::DisplayMode::MODE_SHADED_WITH_EDGES;
            if (object->Attributes()->FaceBoundaryDraw() != showFaceBounds) {
                object->Attributes()->SetFaceBoundaryDraw(showFaceBounds);
                Handle(Prs3d_LineAspect) visibleLineAspect =
                    new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 2.0);
                object->Attributes()->SetFaceBoundaryAspect(visibleLineAspect);
                auto aisLink = Handle_AIS_ConnectedInteractive::DownCast(object);
                if (aisLink && aisLink->HasConnection()) {
                    aisLink->ConnectedTo()->Attributes()->SetFaceBoundaryDraw(showFaceBounds);
                    aisLink->ConnectedTo()->Redisplay(true);
                } else {
                    object->Redisplay(true /*AllModes*/);
                }
            }
            
        }
    };
    for (const auto &object : m_loadedObjects)
    {
        if (m_context->IsDisplayed(object))
        {
            m_context->Redisplay(object, false);
        }
        else
        {
            m_context->Display(object, object->DisplayMode(), 0, false);
        }
        reDisplayMode(object);
    }

    auto reDisplayInterference = [&](const Handle(AIS_InteractiveObject)& object, AIS_DisplayMode displayMode = AIS_DisplayMode::AIS_Shaded){
        const bool onEntry_AutoActivateSelection = m_context->GetAutoActivateSelection();
        object->Attributes()->SetIsoOnTriangulation(true);
        if (m_context->IsDisplayed(object))
        {
            m_context->Redisplay(object, false);
        }
        else
        {
            m_context->Display(object, object->DisplayMode(), 0, false);
        }
        m_context->SetDisplayMode( object, displayMode, false ) ;
        m_context->SetDisplayPriority(object, 10);
    };

    auto reDisplayInterferenceBoundingBox = [&](const Handle(AIS_InteractiveObject)& object, AIS_DisplayMode displayMode = AIS_DisplayMode::AIS_Shaded){
        if (m_context->IsDisplayed(object))
        {
            m_context->Redisplay(object, false);
        }
        else
        {
            m_context->Display(object, object->DisplayMode(), 0, false);
        }
        m_context->SetDisplayMode( object, displayMode, false ) ;
        m_context->SetDisplayPriority(object, 10);
    };

    for( const auto &object : m_interferenceObjects )
    {
        if (const auto objectImpl = std::reinterpret_pointer_cast<View::InterfereceImpl>(object))
        {
            reDisplayInterference(objectImpl->m_object);
            if( objectImpl->m_boundingBox )
            {
                reDisplayInterferenceBoundingBox(objectImpl->m_boundingBox, AIS_DisplayMode::AIS_WireFrame);
            }
        }
    }

    m_context->UpdateCurrentViewer() ;
    m_view->Redraw();
}

void OCCView::viewfit()
{
    // Create a copy of the camera *before* fitting, this is our starting point.
    Handle(Graphic3d_Camera) aCamStart = new Graphic3d_Camera();
    aCamStart->Copy(m_view->Camera());

    // This modifies the view's internal camera to the "fitted" state.
    m_view->FitAll(0.01, false);

    // Create a copy of the new (end) camera state.
    Handle(Graphic3d_Camera) aCamEnd = new Graphic3d_Camera();
    aCamEnd->Copy(m_view->Camera());

    // Restore the view's camera to its original state, so the animation starts from the correct position.
    m_view->SetCamera(aCamStart);

    // Start the animation from the start state to the end state.
    animateCamera(aCamStart, aCamEnd);

    updateView();
}

void OCCView::viewUpdate()
{
    if (!m_view.IsNull()) {
        m_view->Update();
    }
    updateView();
}

void OCCView::viewIsometric()
{
    animateViewChange(V3d_TypeOfOrientation_Zup_AxoRight);
    updateView();
}

void OCCView::viewTop()
{
    animateViewChange(V3d_TypeOfOrientation_Zup_Top);
    updateView();
}

void OCCView::viewBottom()
{
    animateViewChange(V3d_TypeOfOrientation_Zup_Bottom);
    updateView();
}

void OCCView::viewLeft()
{
    animateViewChange(V3d_TypeOfOrientation_Zup_Left);
    updateView();
}

void OCCView::viewRight()
{
    animateViewChange(V3d_TypeOfOrientation_Zup_Right);
    updateView();
}

void OCCView::viewFront()
{
    animateViewChange(V3d_TypeOfOrientation_Zup_Front);
    updateView();
}

void OCCView::viewBack()
{
    animateViewChange(V3d_TypeOfOrientation_Zup_Back);
    updateView();
}

void OCCView::setDisplayMode( View::DisplayMode mode )
{
    m_displayMode = mode ;
    reDraw();
    updateView();
}

void OCCView::createWorkPlane(double x, double y, double z, double dx, double dy, double dz)
{
    if( !m_viewer->IsActive() ){
        m_viewer->ActivateGrid(m_workPlane.m_gridType, m_workPlane.m_gridDrawMode); // show grid grand
    }
    auto isValidDirection =[](double dx, double dy, double dz) {
        try {
            gp_Dir dir(dx, dy, dz);
            return true;
        }
        catch (const Standard_ConstructionError&) {
            return false;
        };
    };
    if( isValidDirection(dx, dy, dz) )
        m_viewer->SetPrivilegedPlane({ gp_Pnt(x, y, z), gp_Dir(dx, dy, dz) });
    reDraw();
    updateView();
}

bool OCCView::isWorkPlaneActive() const
{
    return m_viewer->IsActive();
}

void OCCView::deactivateWorkPlane()
{
    if( m_viewer->IsActive() ){
        m_viewer->DeactivateGrid();
    }
    reDraw();
    updateView();
}

void OCCView::animateCamera(const Handle(Graphic3d_Camera) & theCamStart, const Handle(Graphic3d_Camera) & theCamEnd)
{
    myViewAnimation->SetView(m_view);
    myViewAnimation->SetCameraStart(theCamStart);
    myViewAnimation->SetCameraEnd(theCamEnd);
    myViewAnimation->StartTimer(0, 1, true, false);
    myViewAnimation->Start(false);
}

void OCCView::animateViewChange(V3d_TypeOfOrientation theOrientation)
{
    Handle(Graphic3d_Camera) aCamStart = new Graphic3d_Camera();
    aCamStart->Copy(m_view->Camera());

    m_view->SetProj(theOrientation);

    Handle(Graphic3d_Camera) aCamEnd = new Graphic3d_Camera();
    aCamEnd->Copy(m_view->Camera());

    m_view->SetCamera(aCamStart);

    animateCamera(aCamStart, aCamEnd);
}

void OCCView::addClippingPlane(const gp_Pnt& point, const gp_Dir& normal, const bool isOn,const bool isCap) 
{
    std::shared_ptr<View::ClippingPlane> clipPlane { nullptr };
    if (m_clippingPlanes.empty())
    {
        clipPlane = std::make_shared<View::ClippingPlane>(new Graphic3d_ClipPlane(gp_Pln(point, normal)), isOn, isCap);
        m_clippingPlanes.emplace_back(clipPlane);
        m_view->AddClipPlane(clipPlane->m_clipPlane);
    }
    else
    {
        clipPlane = m_clippingPlanes.at(0);
        clipPlane->m_clipPlane->SetEquation(gp_Pln(point, normal));
        clipPlane->m_clipPlane->SetOn(isOn);
        clipPlane->m_clipPlane->SetCapping(isCap);
    }

    viewUpdate();
}

void OCCView::setClippingPlaneIsOn(const bool isOn)
{
    if (m_clippingPlanes.empty()){
        return;
    }
    m_clippingPlanes.at(0)->m_clipPlane->SetOn(isOn);
    viewUpdate();
}

void OCCView::setCappingPlaneIsCap(const bool isCap)
{
    if (m_clippingPlanes.empty()){
        return;
    }
    m_clippingPlanes.at(0)->m_clipPlane->SetCapping(isCap);

    viewUpdate();
}

void OCCView::explosion(const double theFactor) 
{
    auto applyExplosion = [&](const std::vector<Handle(AIS_InteractiveObject)> &objectList, const double distanceMultiplier = 50.0)
    {
        auto computeShapeCenter = [](const TopoDS_Shape &shape) -> gp_Pnt
        {
            Bnd_Box bbox;
            BRepBndLib::Add(shape, bbox);
            Standard_Real xMin{}, yMin{}, zMin{}, xMax{}, yMax{}, zMax{};
            bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
            return gp_Pnt((xMin + xMax) / 2.0, (yMin + yMax) / 2.0, (zMin + zMax) / 2.0);
        };

        gp_Pnt globalCenter(0, 0, 0);
        if (!objectList.empty())
        {
            Bnd_Box globalBox;
            for (const auto &object : objectList)
            {
                const auto aisShape = Handle(AIS_Shape)::DownCast(object);
                const auto &shape = aisShape->Shape();
                BRepBndLib::Add(shape, globalBox);
            }
            Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
            globalBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
            globalCenter = gp_Pnt((xMin + xMax) / 2.0, (yMin + yMax) / 2.0, (zMin + zMax) / 2.0);
        }
        
        // transform
        for (const auto &object : objectList)
        {
            const auto aisShape = Handle(AIS_Shape)::DownCast(object);
            const auto &shape = aisShape->Shape();
            gp_Pnt center = computeShapeCenter(shape);
            gp_Vec moveDir(globalCenter, center);
            if (moveDir.Magnitude() > 0.0)
            {
                moveDir.Normalize();
                moveDir *= distanceMultiplier;
            }
            gp_Trsf currentTrsf = aisShape->LocalTransformation();
            gp_Trsf transform;
            transform.SetTranslation(moveDir);
            currentTrsf.Multiply(transform);
            aisShape->SetLocalTransformation(currentTrsf);
        }

    };

    applyExplosion(m_loadedObjects, theFactor * 100);

    reDraw();
    updateView();
}

void OCCView::setMouseMode(const View::MouseMode mode)
{
    m_mouseMode = mode;
}

void OCCView::patternLinear(const TopoDS_Shape &baseShape, const gp_Vec &direction,
                            Standard_Real spacing, Standard_Integer count)
{
    for (Standard_Integer i = 1; i < count; i++)
    {
        gp_Trsf transform;
        gp_Vec translation = direction.Normalized() * (spacing * i);
        transform.SetTranslation(translation);

        BRepBuilderAPI_Transform transformer(baseShape, transform);
        if (transformer.IsDone())
        {
            Handle(AIS_Shape) anAisShape = new AIS_Shape(transformer.Shape());
            setShape(anAisShape);
        }
    }
    reDraw();
}

void OCCView::patternCircular(const TopoDS_Shape &baseShape, const gp_Ax1 &axis,
                              Standard_Real angleStep, Standard_Integer count)
{
    for (Standard_Integer i = 1; i < count; i++)
    {
        gp_Trsf transform;
        Standard_Real angle = angleStep * i;
        transform.SetRotation(axis, angle);

        BRepBuilderAPI_Transform transformer(baseShape, transform);
        if (transformer.IsDone())
        {
            Handle(AIS_Shape) anAisShape = new AIS_Shape(transformer.Shape());
            setShape(anAisShape);
        }
    }
    reDraw();
}

void OCCView::mirrorByPlane(const TopoDS_Shape &shape, const gp_Pln &mirrorPlane)
{
    gp_Trsf mirrorTransform;
    mirrorTransform.SetMirror(mirrorPlane.Position().Ax2());
    BRepBuilderAPI_Transform transformer(shape, mirrorTransform);

    if (transformer.IsDone()) {
        auto result =  transformer.Shape();
        for(auto &aisShape : m_loadedObjects) {
            auto target = Handle(AIS_Shape)::DownCast(aisShape);
            if (target->Shape() == shape) {
                target->SetShape(result);
                continue;
                //aisShape = mirrorAisShape;
            }
        }
    }
    reDraw();
}

void OCCView::mirrorByAxis(const TopoDS_Shape &shape, const gp_Ax1 &mirrorAxis)
{
    gp_Trsf mirrorTransform;
    mirrorTransform.SetMirror(mirrorAxis);

    BRepBuilderAPI_Transform transformer(shape, mirrorTransform);

    if (transformer.IsDone()) {
        auto result =  transformer.Shape();
        for(auto &aisShape : m_loadedObjects) {
            auto target = Handle(AIS_Shape)::DownCast(aisShape);
            if (target->Shape() == shape) {
                target->SetShape(result);
                continue;
                //aisShape = mirrorAisShape;
            }
        }
    }
    reDraw();
}

TopoDS_Shape OCCView::shell(const TopoDS_Shape &box, const TopTools_ListOfShape &facesToRemove,
                    const Standard_Real thickness, const Standard_Real tolerance)
{
    BRepOffsetAPI_MakeThickSolid thickSolid;
    thickSolid.MakeThickSolidByJoin(
        box,
        facesToRemove,
        thickness,
        tolerance
    );
    if (thickSolid.IsDone()) {
        TopoDS_Shape shellShape = thickSolid.Shape();
        return shellShape;
    }
    return {};
}

void OCCView::OnSubviewChanged(const Handle(AIS_InteractiveContext) &, const Handle(V3d_View) &,
                               const Handle(V3d_View) & theNewView)
{
    myFocusView = theNewView;
}
