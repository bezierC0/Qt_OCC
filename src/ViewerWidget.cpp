#include "ViewerWidget.h"
#include "OCCView.h"
#include <QVBoxLayout>

#include <STEPControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TopExp_Explorer.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <TDataStd_Name.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#include <AIS_Shape.hxx>

namespace {

bool isShapeAssembly( const TDF_Label& lbl )
{
  return XCAFDoc_ShapeTool::IsAssembly( lbl );
}

bool isShapeReference( const TDF_Label& lbl )
{
  return XCAFDoc_ShapeTool::IsReference( lbl );
}

bool isShapeSimple( const TDF_Label& lbl )
{
  return XCAFDoc_ShapeTool::IsSimpleShape( lbl );
}

bool isShapeComponent( const TDF_Label& lbl )
{
  return XCAFDoc_ShapeTool::IsComponent( lbl );
}

bool isShapeCompound( const TDF_Label& lbl )
{
  return XCAFDoc_ShapeTool::IsCompound( lbl );
}

bool isShapeSub( const TDF_Label& lbl )
{
  return XCAFDoc_ShapeTool::IsSubShape( lbl );
}


TDF_LabelSequence shapeComponents( const TDF_Label& lbl )
{
  TDF_LabelSequence seq;
  XCAFDoc_ShapeTool::GetComponents( lbl, seq );
  return seq;
}

TDF_Label shapeReferred( const TDF_Label& lbl )
{
  TDF_Label referred;
  XCAFDoc_ShapeTool::GetReferredShape( lbl, referred );
  return referred;
}

// mayo xcaf.cpp deepBuildAssemblyTree
uint32_t deepBuildAssemblyTree( uint32_t parentNode, const TDF_Label& label )
{

  //const uint32_t node = m_modelTree->appendChild( parentNode, label );
  if ( isShapeAssembly( label ) ) 
  { 
      Handle_TDataStd_Name attrName;
      if ( label.FindAttribute( TDataStd_Name::GetID(), attrName ) ) {
          //std::cout << attrName->Get() << std::endl;
      }
      std::cout << " ShapeAssembly : " << attrName->Get() << std::endl;
      for ( const TDF_Label& child : shapeComponents( label ) )
          deepBuildAssemblyTree( 0, child );
  }
  else if ( isShapeReference( label ) ) 
  {
      Handle_TDataStd_Name attrName;
      if ( label.FindAttribute( TDataStd_Name::GetID(), attrName ) ) {
        //std::cout << attrName->Get() << std::endl;
      }
      std::cout << "ShapeReference : " << attrName->Get() << std::endl;
      const TDF_Label referred = shapeReferred( label );
      deepBuildAssemblyTree( 0, referred );
  }

  return 0;
}
}


ViewerWidget::ViewerWidget(QWidget* parent) : QWidget(parent)
{
    m_occView = new OCCView(this);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_occView);
    layout->setMargin(0);

    m_doc = std::make_shared<Document>();
}

ViewerWidget::~ViewerWidget()
{
    if (m_occView)
    {
        delete m_occView;
        m_occView = nullptr;
    }
}



void ViewerWidget::loadModel(const QString& filename) const
{
    
    if (filename.endsWith(".step") || filename.endsWith(".stp"))
    {
#if 0
        STEPControl_Reader reader;
        IFSelect_ReturnStatus status = reader.ReadFile(filename.toStdString().c_str());
        if ( status == IFSelect_RetDone )
        {
            reader.TransferRoots();
            /*const int nbRoots = reader.NbRootsForTransfer();
            for (int i = 1; i <= nbRoots; ++i)
            {

                TopoDS_Shape subShape = reader.Shape(i);
                if (subShape.IsNull())
                    continue;
                shapeList.emplace_back( std::move(subShape) );

            }*/
            for (TopExp_Explorer exp(reader.OneShape(), TopAbs_SOLID); exp.More(); exp.Next())
            {
                TopoDS_Shape part = exp.Current();
                if (!part.IsNull())
                    m_doc->m_list.emplace_back(std::move(part));
            }
        }

#else
        STEPCAFControl_Reader reader;
        IFSelect_ReturnStatus status = reader.ReadFile(filename.toStdString().c_str());
        if (status != IFSelect_RetDone)
        {
            std::cout << "STEP file read failed";

        }
        Handle(TDocStd_Document) doc;
        Handle(XCAFDoc_ShapeTool) shapeTool;
        Handle(XCAFDoc_ColorTool) colorTool;

        Handle(XCAFApp_Application)::DownCast(XCAFApp_Application::GetApplication())->NewDocument("BinXCAF", doc);

        if (!reader.Transfer(doc))
        {
            std::cout << "STEP transfer to document failed";
            return;
        }

        shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
        colorTool = XCAFDoc_DocumentTool::ColorTool(doc->Main());

        TDF_LabelSequence labels;
        shapeTool->GetFreeShapes(labels); 
        for ( const TDF_Label& label : labels ) {
            deepBuildAssemblyTree( 0, label );
        }

        for (Standard_Integer i = 1; i <= labels.Length(); ++i)
        {
            TDF_Label label = labels.Value(i);

            TopoDS_Shape shape = shapeTool->GetShape(label);
            if (shape.IsNull())
                continue;

            auto nc = shape.NbChildren();

            deepBuildAssemblyTree( 0, label );

            auto GetShapeType = [&]( const TDF_Label& label ) -> int
            {
                int type = 0;
                if ( XCAFDoc_ShapeTool::IsAssembly( label ) ) {
                    type = 1;
                }
                else if ( XCAFDoc_ShapeTool::IsReference( label ) ) {
                    type = 2;
                }
                else if ( XCAFDoc_ShapeTool::IsSimpleShape( label ) ) {
                    type = 3;
                }
                else if ( XCAFDoc_ShapeTool::IsComponent( label ) ) {
                    type = 4;
                }
                else if ( XCAFDoc_ShapeTool::IsCompound( label ) ) {
                    type = 5;
                }
                else if ( XCAFDoc_ShapeTool::IsSubShape( label ) ) {
                    type = 6;
                }
                return type ;
            };


            /*TCollection_ExtendedString name;
            if (XCAFDoc_ShapeTool::GetShapeLabel(doc->Main(), shape, label))
            {
                name = shapeTool->GetShapeName(label);
            }

            QString partName = QString::fromUtf16((const char16_t*)name.ToExtString());*/

            if ( 1 == GetShapeType( label ) ) {
              TDF_LabelSequence seq;
              XCAFDoc_ShapeTool::GetComponents( label, seq );
              for ( const TDF_Label& childLabel : seq ) {
                  auto childType = GetShapeType( childLabel );
                  Quantity_Color color;

                  auto shapeColor = [&]( const TDF_Label& lbl ) 
                  {
                    Handle_XCAFDoc_ColorTool tool = colorTool;
                    Quantity_Color color = {};
                    if ( !tool )
                      return color;

                    if ( tool->GetColor( lbl, XCAFDoc_ColorGen, color ) )
                      return color;

                    if ( tool->GetColor( lbl, XCAFDoc_ColorSurf, color ) )
                      return color;

                    if ( tool->GetColor( lbl, XCAFDoc_ColorCurv, color ) )
                      return color;

                    return color;
                  };

                  color = shapeColor( childLabel ) ;
                  std::cout << color.Red() << " " << color.Green() << " " << color.Blue() << std::endl;
                  std::cout << childType << std::endl;
              }
            }

            
        }
#endif
    }
    else if (filename.endsWith(".iges") || filename.endsWith(".igs"))
    {
        IGESControl_Reader reader;
        if (reader.ReadFile(filename.toStdString().c_str()) == IFSelect_RetDone)
        {
            reader.TransferRoots();
            int nbRoots = reader.NbRootsForTransfer();
            for (int i = 1; i <= nbRoots; ++i)
            {
                TopoDS_Shape subShape = reader.Shape(i);
                if (subShape.IsNull())
                    continue;
                m_doc->m_list.emplace_back(std::move(subShape));
            }
        }
    }

    for ( const auto& subShape : m_doc->m_list)
    {
        if (!subShape.IsNull())
        {

            Handle(AIS_Shape) aisShape = new AIS_Shape(subShape);
            m_occView->setShape(aisShape);
        }
    }
    m_occView->reDraw();

    
}

void ViewerWidget::viewFit()
{
    m_occView->fit();
}

void ViewerWidget::checkInterference()
{
    for (size_t i = 0; i < m_doc->m_list.size(); ++i)
    {
        for (size_t j = i + 1; j < m_doc->m_list.size(); ++j)
        {

            const TopoDS_Shape& shapeA = m_doc->m_list[i];
            const TopoDS_Shape& shapeB = m_doc->m_list[j];

            BRepAlgoAPI_Common commonOp(shapeA, shapeB);
            commonOp.Build();

            if (commonOp.IsDone())
            {
                const TopoDS_Shape& result = commonOp.Shape();
                if (!result.IsNull())
                {
                    std::cout << "Shape " << i << " intersects with Shape " << j << std::endl;


                    Handle(AIS_Shape) aisResult = new AIS_Shape(result);
                    aisResult->SetColor(Quantity_NOC_RED);
                    aisResult->SetDisplayMode(AIS_Shaded);
                    //m_occView->displayShape(aisResult);
                }
            }
        }
    }
}

void ViewerWidget::clipping( const gp_Dir& normal, const gp_Pnt& point, const bool isOn )
{
  const Handle( Graphic3d_ClipPlane ) clipPlane = new Graphic3d_ClipPlane( gp_Pln( point, normal ) );
  clipPlane->SetCapping( false );
  clipPlane->SetOn( isOn );

  m_occView->View()->AddClipPlane( clipPlane );
  m_occView->View()->Update();
}

void ViewerWidget::explosion()
{
    auto computeShapeCenter = [](const TopoDS_Shape& shape) -> gp_Pnt
        {
            Bnd_Box bbox;
            BRepBndLib::Add(shape, bbox);
            Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
            bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
            return gp_Pnt((xMin + xMax) / 2.0, (yMin + yMax) / 2.0, (zMin + zMax) / 2.0);
        };

    auto applyExplosion = [&](const std::vector<TopoDS_Shape>& shapeList,
        const double distanceMultiplier = 50.0) -> std::vector<TopoDS_Shape>
        {
            std::vector<TopoDS_Shape> explodedShapes;

            gp_Pnt globalCenter(0, 0, 0);
            if (!shapeList.empty())
            {
                Bnd_Box globalBox;
                for (const TopoDS_Shape& shape : shapeList)
                {
                    BRepBndLib::Add(shape, globalBox);
                }
                Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
                globalBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
                globalCenter = gp_Pnt((xMin + xMax) / 2.0, (yMin + yMax) / 2.0, (zMin + zMax) / 2.0);
            }

            for (const TopoDS_Shape& shape : shapeList)
            {
                gp_Pnt center = computeShapeCenter(shape);
                gp_Vec moveDir(globalCenter, center);
                if (moveDir.Magnitude() > 0.0)
                {
                    moveDir.Normalize();
                    moveDir *= distanceMultiplier;
                }

                gp_Trsf transform;
                transform.SetTranslation(moveDir);
                BRepBuilderAPI_Transform transformer(shape, transform, true);
                explodedShapes.emplace_back(transformer.Shape());
            }

            return explodedShapes;
        };


    const auto explodedShapes = applyExplosion(m_doc->m_list, 80.0);

    m_occView->clearShape();
    for (const TopoDS_Shape& shape : explodedShapes)
    {
        Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
        aisShape->SetDisplayMode(AIS_Shaded);
        aisShape->SetColor(Quantity_NOC_SKYBLUE);
        m_occView->setShape( aisShape );
    }
    m_occView->reDraw();
}
