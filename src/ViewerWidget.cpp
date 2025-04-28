#include "ViewerWidget.h"
#include "OCCView.h"
#include <QVBoxLayout>
#include <STEPControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <AIS_Shape.hxx>

ViewerWidget::ViewerWidget(QWidget* parent) : QWidget(parent) {
    m_occView = new OCCView(this);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_occView);
    layout->setMargin(0);
}

void ViewerWidget::loadModel(const QString& filename) const
{
    TopoDS_Shape shape;
    if ( filename.endsWith( ".step" ) || filename.endsWith( ".stp" ) ) {
      STEPControl_Reader reader;
      if ( reader.ReadFile( filename.toStdString().c_str() ) == IFSelect_RetDone ) {
        reader.TransferRoots();
        shape = reader.OneShape();
      }
    }
    else if ( filename.endsWith( ".iges" ) || filename.endsWith( ".igs" ) ) {
      IGESControl_Reader reader;
      reader.ReadFile( filename.toStdString().c_str() );
      reader.TransferRoots();
      shape = reader.OneShape();
    }

    if ( !shape.IsNull() ) {
      //m_loadedShape = shape;
      Handle( AIS_Shape ) aisShape = new AIS_Shape( shape );
      m_occView->setShape( aisShape ) ;
    }
}

void ViewerWidget::setTopView() {
    // m_view->SetProj(V3d_Xneg);
    // m_view->FitAll();
}

void ViewerWidget::checkInterference()
{

}
