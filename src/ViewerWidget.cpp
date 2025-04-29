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
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#include <AIS_Shape.hxx>


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
#if 1
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
        Handle(XCAFApp_Application)::DownCast(XCAFApp_Application::GetApplication())->NewDocument("MDTV-XCAF", doc);

        if (!reader.Transfer(doc))
        {
            std::cout << "STEP transfer to document failed";
            return;
        }

        
        shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
        colorTool = XCAFDoc_DocumentTool::ColorTool(doc->Main());

        TDF_LabelSequence labels;
        shapeTool->GetFreeShapes(labels); 

        for (Standard_Integer i = 1; i <= labels.Length(); ++i)
        {
            TDF_Label label = labels.Value(i);

            TopoDS_Shape shape = shapeTool->GetShape(label);
            if (shape.IsNull())
                continue;


            /*TCollection_ExtendedString name;
            if (XCAFDoc_ShapeTool::GetShapeLabel(doc->Main(), shape, label))
            {
                name = shapeTool->GetShapeName(label);
            }

            QString partName = QString::fromUtf16((const char16_t*)name.ToExtString());*/


            Quantity_Color color;

            if (colorTool->GetColor(label, color))
            {

                double r = color.Red();
                double g = color.Green();
                double b = color.Blue();
                //qDebug() << "Part" << partName << "Color:" << r << g << b;
            }
            else
            {
                //qDebug() << "Part" << partName << "no color";
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

void ViewerWidget::setTopView()
{
    // m_view->SetProj(V3d_Xneg);
    // m_view->FitAll();
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

void ViewerWidget::clipping()
{
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
        double distanceMultiplier = 50.0) -> std::vector<TopoDS_Shape>
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

                gp_Trsf trsf;
                trsf.SetTranslation(moveDir);
                BRepBuilderAPI_Transform transformer(shape, trsf, true);
                explodedShapes.emplace_back(transformer.Shape());
            }

            return explodedShapes;
        };


    auto explodedShapes = applyExplosion(m_doc->m_list, 80.0);

    for (const TopoDS_Shape& shape : explodedShapes)
    {
        Handle(AIS_Shape) ais = new AIS_Shape(shape);
        ais->SetDisplayMode(AIS_Shaded);
        ais->SetColor(Quantity_NOC_SKYBLUE);
        //m_occView->displayShape(ais);
    }
}
