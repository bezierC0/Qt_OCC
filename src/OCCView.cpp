#include "OCCView.h"
#include <OpenGl_GraphicDriver.hxx>
#include <WNT_Window.hxx>

OCCView::OCCView(QWidget* parent) : QOpenGLWidget(parent) {

    QSurfaceFormat fmt;
    fmt.setDepthBufferSize( 24 );
    fmt.setStencilBufferSize( 8 );
    fmt.setVersion( 3, 3 );
    fmt.setProfile( QSurfaceFormat::CoreProfile );
    QSurfaceFormat::setDefaultFormat( fmt );
    m_wntWindow = new WNT_Window( (Aspect_Handle)winId() );
    
    Handle( Aspect_DisplayConnection ) aDisplayConnection = new Aspect_DisplayConnection();
    m_graphicDriver = new OpenGl_GraphicDriver( aDisplayConnection );

    m_viewer = new V3d_Viewer(m_graphicDriver);
    m_viewer->SetDefaultLights();
    m_viewer->SetLightOn();

    m_context = new AIS_InteractiveContext(m_viewer);
    m_view = m_viewer->CreateView();
    m_view->SetBackgroundColor(Quantity_NOC_WHITE);

    setAutoFillBackground( false ); 
    setAttribute( Qt::WA_NoSystemBackground );
    setAttribute( Qt::WA_OpaquePaintEvent );
}

Handle(V3d_View) OCCView::getView() const { return m_view; }
Handle(AIS_InteractiveContext) OCCView::getContext() const { return m_context; }

void OCCView::initializeGL()
{
    if ( !m_view.IsNull() ) {
      //m_view->MustBeResized();
      //m_view->SetProj( V3d_TypeOfOrientation_Zup_AxoRight );
      //m_view->FitAll();
      
        m_view->SetWindow( m_wntWindow );
        m_view->MustBeResized();
        m_view->TriedronDisplay( Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GRAY, 0.08, V3d_ZBUFFER );
        m_view->SetBackgroundColor( Quantity_NOC_WHITE );
        m_view->ZFitAll();
        //m_view->MustBeResized();
        //m_view->TriedronDisplay( Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GRAY, 0.08, V3d_ZBUFFER );
        //m_view->ZFitAll();
        //m_view->Redraw();
    }
}

void OCCView::paintEvent(QPaintEvent*) {
    if (!m_view.IsNull() && !m_wntWindow.IsNull() ) {
        m_view->Redraw();
    }
}

void OCCView::resizeEvent(QResizeEvent* e) {
    QOpenGLWidget::resizeEvent( e );  
    if (!m_view.IsNull()) {
        m_view->MustBeResized();
        m_view->Redraw();
    }
}
