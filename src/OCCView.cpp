#ifdef _WIN32
#include <windows.h>
#endif
#include <OpenGl_Context.hxx>

#include "OCCView.h"

#include "OcctGlTools.h"

#include <Standard_WarningsDisable.hxx>
#include <QApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <Standard_WarningsRestore.hxx>

#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <Message.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OpenGl_FrameBuffer.hxx>

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
        case Qt::Key_A: return Aspect_VKey_A;
        case 1048: // ru
        case Qt::Key_B: return Aspect_VKey_B;
        case 1057: // ru
        case Qt::Key_C: return Aspect_VKey_C;
        case 1042: // ru
        case Qt::Key_D: return Aspect_VKey_D;
        case 1059: // ru
        case Qt::Key_E: return Aspect_VKey_E;
        case 1040: // ru
        case Qt::Key_F: return Aspect_VKey_F;
        case Qt::Key_G: return Aspect_VKey_G;
        case Qt::Key_H: return Aspect_VKey_H;
        case Qt::Key_I: return Aspect_VKey_I;
        case Qt::Key_J: return Aspect_VKey_J;
        case Qt::Key_K: return Aspect_VKey_K;
        case 1044: // ru
        case Qt::Key_L: return Aspect_VKey_L;
        case Qt::Key_M: return Aspect_VKey_M;
        case Qt::Key_N: return Aspect_VKey_N;
        case Qt::Key_O: return Aspect_VKey_O;
        case Qt::Key_P: return Aspect_VKey_P;
        case 1049: // ru
        case Qt::Key_Q: return Aspect_VKey_Q;
        case 1050: // ru
        case Qt::Key_R: return Aspect_VKey_R;
        case 1067: // ru
        case Qt::Key_S: return Aspect_VKey_S;
        case 1045: // ru
        case Qt::Key_T: return Aspect_VKey_T;
        case Qt::Key_U: return Aspect_VKey_U;
        case 1052: // ru
        case Qt::Key_V: return Aspect_VKey_V;
        case 1062: // ru
        case Qt::Key_W: return Aspect_VKey_W;
        case 1063: // ru
        case Qt::Key_X: return Aspect_VKey_X;
        case Qt::Key_Y: return Aspect_VKey_Y;
        case 1071: // ru
        case Qt::Key_Z: return Aspect_VKey_Z;
            //
        case Qt::Key_0: return Aspect_VKey_0;
        case Qt::Key_1: return Aspect_VKey_1;
        case Qt::Key_2: return Aspect_VKey_2;
        case Qt::Key_3: return Aspect_VKey_3;
        case Qt::Key_4: return Aspect_VKey_4;
        case Qt::Key_5: return Aspect_VKey_5;
        case Qt::Key_6: return Aspect_VKey_6;
        case Qt::Key_7: return Aspect_VKey_7;
        case Qt::Key_8: return Aspect_VKey_8;
        case Qt::Key_9: return Aspect_VKey_9;
            //
        case Qt::Key_F1:        return Aspect_VKey_F1;
        case Qt::Key_F2:        return Aspect_VKey_F2;
        case Qt::Key_F3:        return Aspect_VKey_F3;
        case Qt::Key_F4:        return Aspect_VKey_F4;
        case Qt::Key_F5:        return Aspect_VKey_F5;
        case Qt::Key_F6:        return Aspect_VKey_F6;
        case Qt::Key_F7:        return Aspect_VKey_F7;
        case Qt::Key_F8:        return Aspect_VKey_F8;
        case Qt::Key_F9:        return Aspect_VKey_F9;
        case Qt::Key_F10:       return Aspect_VKey_F10;
        case Qt::Key_F11:       return Aspect_VKey_F11;
        case Qt::Key_F12:       return Aspect_VKey_F12;
            //
        case Qt::Key_Up:        return Aspect_VKey_Up;
        case Qt::Key_Left:      return Aspect_VKey_Left;
        case Qt::Key_Right:     return Aspect_VKey_Right;
        case Qt::Key_Down:      return Aspect_VKey_Down;
        case Qt::Key_Plus:      return Aspect_VKey_Plus;
        case Qt::Key_Minus:     return Aspect_VKey_Minus;
        case Qt::Key_Equal:     return Aspect_VKey_Equal;
        case Qt::Key_PageDown:  return Aspect_VKey_PageDown;
        case Qt::Key_PageUp:    return Aspect_VKey_PageUp;
        case Qt::Key_Home:      return Aspect_VKey_Home;
        case Qt::Key_End:       return Aspect_VKey_End;
        case Qt::Key_Escape:    return Aspect_VKey_Escape;
        case Qt::Key_Back:      return Aspect_VKey_Back;
        case Qt::Key_Enter:     return Aspect_VKey_Enter;
        case Qt::Key_Backspace: return Aspect_VKey_Backspace;
        case Qt::Key_Space:     return Aspect_VKey_Space;
        case Qt::Key_Delete:    return Aspect_VKey_Delete;
        case Qt::Key_Tab:       return Aspect_VKey_Tab;
        case 1025:
        case Qt::Key_QuoteLeft: return Aspect_VKey_Tilde;
            //
        case Qt::Key_Shift:     return Aspect_VKey_Shift;
        case Qt::Key_Control:   return Aspect_VKey_Control;
        case Qt::Key_Alt:       return Aspect_VKey_Alt;
        case Qt::Key_Menu:      return Aspect_VKey_Menu;
        case Qt::Key_Meta:      return Aspect_VKey_Meta;
        default:                return Aspect_VKey_UNKNOWN;
        }
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
    void BindBuffer(const Handle(OpenGl_Context)& theGlCtx) override
    {
        OpenGl_FrameBuffer::BindBuffer(theGlCtx);
        theGlCtx->SetFrameBufferSRGB(true, false);
    }

    //! Make this FBO as drawing target in context.
    void BindDrawBuffer(const Handle(OpenGl_Context)& theGlCtx) override
    {
        OpenGl_FrameBuffer::BindDrawBuffer(theGlCtx);
        theGlCtx->SetFrameBufferSRGB(true, false);
    }

    //! Make this FBO as reading source in context.
    void BindReadBuffer(const Handle(OpenGl_Context)& theGlCtx) override
    {
        OpenGl_FrameBuffer::BindReadBuffer(theGlCtx);
    }
};

OCCView::OCCView(QWidget* theParent)
    : QOpenGLWidget(theParent),
    myIsCoreProfile(true),
    m_interactionMode(InteractionMode::Highlight)
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
    m_viewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines); // show grid grand

    // create AIS context
    m_context = new AIS_InteractiveContext(m_viewer);

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
    m_view->ChangeRenderingParams().CollectedStats = (Graphic3d_RenderingParams::PerfCounters)
        (Graphic3d_RenderingParams::PerfCounters_FrameRate
            | Graphic3d_RenderingParams::PerfCounters_Triangles);

    // Qt widget setup
    setMouseTracking(true);
    setBackgroundRole(QPalette::NoRole); // or NoBackground
    setFocusPolicy(Qt::StrongFocus); // set focus policy to threat QContextMenuEvent from keyboard
    setUpdatesEnabled(true);
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

    // OpenGL setup managed by Qt
    QSurfaceFormat aGlFormat;
    aGlFormat.setDepthBufferSize(24);
    aGlFormat.setStencilBufferSize(8);
    //aGlFormat.setOption (QSurfaceFormat::DebugContext, true);
    aDriver->ChangeOptions().contextDebug = aGlFormat.testOption(QSurfaceFormat::DebugContext);
    //aGlFormat.setOption (QSurfaceFormat::DeprecatedFunctions, true);
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

void OCCView::setInteractionMode(InteractionMode theMode)
{
    if (m_interactionMode == theMode)
    {
        return;
    }

    m_interactionMode = theMode;
    m_context->Deactivate(); // Deactivate all previous modes

    if (m_interactionMode == InteractionMode::Select)
    {
        //m_context->Activate(AIS_Shape::SelectionMode(TopAbs_FACE));
        //m_context->Activate(AIS_Shape::SelectionMode(TopAbs_EDGE));
        m_context->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
        m_context->Activate(AIS_Shape::SelectionMode(TopAbs_SOLID));
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
    HDC   aWglDevCtx = wglGetCurrentDC();
    HWND  aWglWin = WindowFromDC(aWglDevCtx);
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
        //dumpGlInfo (true, true);
    }
    else
    {
        aWindow = new Aspect_NeutralWindow();
        aWindow->SetVirtual(true);
        aWindow->SetNativeHandle(aNativeWin);
        aWindow->SetSize(aViewSize.x(), aViewSize.y());
        m_view->SetWindow(aWindow, aGlCtx->RenderingContext());
        //dumpGlInfo (true, true);

        m_context->Display(m_navigationView, 0, 0, false);
    }
}


void OCCView::closeEvent(QCloseEvent* theEvent)
{
    theEvent->accept();
}


void OCCView::keyPressEvent(QKeyEvent* theEvent)
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

void OCCView::mousePressEvent(QMouseEvent* theEvent)
{
    if (m_interactionMode == InteractionMode::Select)
    {
        /*const Graphic3d_Vec2i aPnt(theEvent->pos().x(), theEvent->pos().y());
        const AIS_SelectionScheme aScheme = (theEvent->modifiers() & Qt::ShiftModifier) ? AIS_SelectionScheme_Add : AIS_SelectionScheme_Replace;
        m_context->Select(aPnt, m_view, aScheme);*/

        if ( const auto statusOfPick = m_context->SelectDetected( AIS_SelectionScheme_Replace );  statusOfPick == AIS_SOP_OneSelected || statusOfPick == AIS_SOP_SeveralSelected ) {
            auto selectedObject = m_context->FirstSelectedObject();

        }
        updateView();
    }
    else
    {
        QOpenGLWidget::mousePressEvent(theEvent);
        const Graphic3d_Vec2i aPnt(theEvent->pos().x(), theEvent->pos().y());
        const Aspect_VKeyFlags aFlags = qtMouseModifiers2VKeys(theEvent->modifiers());
        if (!m_view.IsNull()
            && UpdateMouseButtons(aPnt,
                qtMouseButtons2VKeys(theEvent->buttons()),
                aFlags,
                false))
        {
            updateView();
        }
    }
}


void OCCView::mouseReleaseEvent(QMouseEvent* theEvent)
{
    QOpenGLWidget::mouseReleaseEvent(theEvent);
    const Graphic3d_Vec2i aPnt(theEvent->pos().x(), theEvent->pos().y());
    const Aspect_VKeyFlags aFlags = qtMouseModifiers2VKeys(theEvent->modifiers());
    if (!m_view.IsNull()
        && UpdateMouseButtons(aPnt,
            qtMouseButtons2VKeys(theEvent->buttons()),
            aFlags,
            false))
    {
        updateView();
    }
}


void OCCView::mouseMoveEvent(QMouseEvent* theEvent)
{
    QOpenGLWidget::mouseMoveEvent(theEvent);
    const Graphic3d_Vec2i aNewPos(theEvent->pos().x(), theEvent->pos().y());
    if (!m_view.IsNull()
        && UpdateMousePosition(aNewPos,
            qtMouseButtons2VKeys(theEvent->buttons()),
            qtMouseModifiers2VKeys(theEvent->modifiers()),
            false))
    {
        updateView();
    }
}


void OCCView::wheelEvent(QWheelEvent* theEvent)
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
        if (!aPickedView.IsNull()
            && aPickedView != myFocusView)
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


void OCCView::updateView()
{
    update();
    //if (window() != NULL) { window()->update(); }
}


void OCCView::paintGL()
{
    if (m_view->Window().IsNull())
    {
        return;
    }
#ifdef _WIN32
    HDC   aWglDevCtx = wglGetCurrentDC();
    HWND  aWglWin = WindowFromDC(aWglDevCtx);
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
    //Handle(OpenGl_GraphicDriver) aDriver = Handle(OpenGl_GraphicDriver)::DownCast (myContext->CurrentViewer()->Driver());
    //Handle(OpenGl_Context) aGlCtx = aDriver->GetSharedContext();
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
    //const QRect aRect = rect(); Graphic3d_Vec2i aViewSizeNew(aRect.right() - aRect.left(), aRect.bottom() - aRect.top());
    Graphic3d_Vec2i aViewSizeNew = aDefaultFbo->GetVPSize();
    Handle(Aspect_NeutralWindow) aWindow = Handle(Aspect_NeutralWindow)::DownCast(m_view->Window());
    aWindow->Size(aViewSizeOld.x(), aViewSizeOld.y());
    if (aViewSizeNew != aViewSizeOld)
    {
        aWindow->SetSize(aViewSizeNew.x(), aViewSizeNew.y());
        m_view->MustBeResized();
        m_view->Invalidate();
        //dumpGlInfo (true, false);

        for (const Handle(V3d_View)& aSubviewIter : m_view->Subviews())
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

void OCCView::handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
    const Handle(V3d_View)& theView)
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
    for ( const auto& object : m_loadedObjects ) {
      m_context->Erase( object,  false );
    }
    m_loadedObjects.clear();
}

void OCCView::setShape(const Handle(AIS_InteractiveObject)& loadedShape)
{
    m_loadedObjects.emplace_back( loadedShape ) ;
}

std::vector<Handle( AIS_InteractiveObject )> OCCView::getShapeObjects() const
{
  return m_loadedObjects;
}

void OCCView::transform()
{
    if ( !m_loadedObjects.empty() ) {
        Handle( AIS_InteractiveObject ) obj = m_loadedObjects.at(0);
        auto aisShape = Handle( AIS_Shape )::DownCast( obj );
        if ( aisShape.IsNull() ) {
            return ;
        }

#if 0
        gp_Trsf trsf;
        trsf.SetTranslation( gp_Vec( 100.0, 0.0, 0.0 ) );
        aisShape->SetLocalTransformation( trsf );
        const TopoDS_Shape& shape = aisShape->Shape();
        TopLoc_Location location = shape.Location();
        gp_Trsf shapeTrsf = location.Transformation();

        std::cout << "\n=== before ===" << std::endl;
        std::cout << "- LocalTransformation: " << aisShape->LocalTransformation().Form() << std::endl;
        std::cout << "- Shape Location Trsf: " << shapeTrsf.Form() << std::endl;
        if ( shapeTrsf.Form() != gp_Identity ) {
            gp_XYZ trans = shapeTrsf.TranslationPart();
            std::cout << "  Translation: (" << trans.X() << ", " << trans.Y() << ", " << trans.Z() << ")" << std::endl;
        }



        TopoDS_Shape newShape = BRepBuilderAPI_Transform( shape, trsf ).Shape();
        aisShape->SetShape( newShape );
        aisShape->ResetTransformation();

        std::cout << "\n=== after ===" << std::endl;
        std::cout << "- LocalTransformation: " << aisShape->LocalTransformation().Form() << std::endl;
        TopLoc_Location newLocation = newShape.Location();
        gp_Trsf newTrsf = newLocation.Transformation();
        std::cout << "- New Shape Location Trsf: " << newTrsf.Form() << std::endl;
        if ( newTrsf.Form() != gp_Identity ) {
            gp_XYZ newTrans = newTrsf.TranslationPart();
            std::cout << "  Translation: (" << newTrans.X() << ", " << newTrans.Y() << ", " << newTrans.Z() << ")" << std::endl;
        }
        Context()->Redisplay( aisShape, true );
#else
        gp_Trsf currentTrsf = aisShape->LocalTransformation();

        gp_Trsf translation;
        translation.SetTranslation( gp_Vec( 100.0, 0.0, 0.0 ) );
        currentTrsf.Multiply( translation );

        aisShape->SetLocalTransformation( currentTrsf );

        Context()->Redisplay( aisShape, true );
#endif
    }
    reDraw();
}

void OCCView::reDraw() const
{
    for ( const auto& object : m_loadedObjects ) {
        // void GraphicsScene::addObject(const GraphicsObjectPtr& object, AddObjectFlags flags)
        const bool onEntry_AutoActivateSelection = m_context->GetAutoActivateSelection();
        object->Attributes()->SetIsoOnTriangulation(true);
        if (m_context->IsDisplayed(object)) {
            m_context->Redisplay(object, false);
        } else {
            m_context->Display(object, object->DisplayMode(), 0, false);
        }
        m_context->SetAutoActivateSelection(onEntry_AutoActivateSelection);
    }
    m_view->Redraw();
}

void OCCView::fit() const
{
    m_view->ZFitAll();
    m_view->FitAll( 0.1, false );
}

void OCCView::clipping() const
{
}

void OCCView::explosion() const
{
    
}

void OCCView::OnSubviewChanged(const Handle(AIS_InteractiveContext)&,
    const Handle(V3d_View)&,
    const Handle(V3d_View)& theNewView)
{
    myFocusView = theNewView;
}
