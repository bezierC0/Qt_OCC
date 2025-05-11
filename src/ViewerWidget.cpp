#include "ViewerWidget.h"
#include "MainWindow.h"
#include "WidgetModelTree.h"
#include "OCCView.h"
#include "Tree.h"

#include <QVBoxLayout>

#include <STEPControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TopExp_Explorer.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFPrs_AISObject.hxx>
#include <TDataStd_Name.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <AIS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>

namespace 
{
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
bool isShape(const TDF_Label& lbl)
{
    return XCAFDoc_ShapeTool::IsShape(lbl);
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
uint32_t deepBuildAssemblyTree(uint32_t parentNode, const TDF_Label& label, Tree<TDF_Label>& modelTree )
{
    const TreeNodeId node = modelTree.appendChild(parentNode, label);
    // GraphicsObjectPtr GraphicsShapeObjectDriver::createObject(const TDF_Label& label) const
    Handle_TDataStd_Name attrName;
    if (label.FindAttribute(TDataStd_Name::GetID(), attrName))
    {
        std::cout << node << "  : " << attrName->Get() << std::endl;
    }
    if (isShapeAssembly(label))
    {
        //if (isShape(label))
        //{
        //    // GraphicsObjectPtr GraphicsShapeObjectDriver::createObject(const TDF_Label& label) const
        //    std::cout << " isShape : \n";
        //}
        for (const TDF_Label& child : shapeComponents(label))
            deepBuildAssemblyTree(node, child, modelTree );
    }
    else if (isShapeReference(label))
    {
        //Handle_TDataStd_Name attrName;
        //if (isShape(label))
        //{
        //    // GraphicsObjectPtr GraphicsShapeObjectDriver::createObject(const TDF_Label& label) const
        //    std::cout << " isShape : \n";
        //}

        const TDF_Label referred = shapeReferred(label);
        deepBuildAssemblyTree(node, referred, modelTree );
    }

    return node;
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


static Tree<TDF_Label> m_modelTree;
void ViewerWidget::loadModel(const QString& filename) const
{
    if (filename.endsWith(".step") || filename.endsWith(".stp"))
    {
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

        int nbRoots = reader.NbRootsForTransfer();

        TDF_LabelSequence labels;
        shapeTool->GetFreeShapes(labels); 
        for ( const TDF_Label& label : labels ) {
            deepBuildAssemblyTree( 0, label, m_modelTree );
        }

        for (const auto& it : m_modelTree.m_vecNode)
        {
            TDF_Label label = it.data;
            if (isShapeReference(label))
            {
                TopoDS_Shape part;

                Handle(XCAFPrs_AISObject) object = new XCAFPrs_AISObject(label);

                // GraphicsObjectPtr GraphicsShapeObjectDriver::createObject(const TDF_Label& label) const
                object->SetDisplayMode(AIS_Shaded);
                object->SetMaterial(Graphic3d_NOM_PLASTER);
                object->Attributes()->SetFaceBoundaryDraw(true);
                object->Attributes()->SetFaceBoundaryAspect(
                    new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 1.)
                );

                m_doc->m_list.emplace_back(object);
            }
        }
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
                Handle(AIS_Shape) aisShape = new AIS_Shape(subShape);
                aisShape->SetDisplayMode(AIS_Shaded);
                aisShape->SetColor(Quantity_NOC_WHITE); 
                m_doc->m_list.emplace_back(aisShape);
            }
        }
    }

    for ( const auto& aisObject : m_doc->m_list)
    {
        m_occView->setShape(aisObject);
    }

    MainWindow* mainWindow = qobject_cast<MainWindow*>( this->window() );
    auto treeWidget = mainWindow->GetModelTreeWidget();
    treeWidget->setModelTree( m_modelTree );
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
            Handle(AIS_InteractiveObject) objA = m_doc->m_list[i];
            Handle(AIS_InteractiveObject) objB = m_doc->m_list[j];

            const auto aisShapeA = Handle(AIS_Shape)::DownCast(objA);
            const auto aisShapeB = Handle(AIS_Shape)::DownCast(objA);

            if (aisShapeA.IsNull() || aisShapeB.IsNull())
                continue;

            const auto& shapeA = aisShapeA->Shape();
            const auto& shapeB = aisShapeB->Shape();

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
                    m_occView->setShape(aisResult);
                }
            }
        }
    }
}

void ViewerWidget::clipping( const gp_Dir& normal, const gp_Pnt& point, const bool isOn )
{
    const Handle(Graphic3d_ClipPlane) clipPlane = new Graphic3d_ClipPlane(gp_Pln(point, normal));
    clipPlane->SetCapping(false);
    clipPlane->SetOn(isOn);

    m_occView->View()->AddClipPlane(clipPlane);
    m_occView->View()->Update();
}

void ViewerWidget::explosion()
{
    auto applyExplosion = [](const std::vector<Handle(AIS_InteractiveObject)>& objectList, const double distanceMultiplier = 50.0)
        {
            auto computeShapeCenter = [](const TopoDS_Shape& shape) -> gp_Pnt
                {
                    Bnd_Box bbox;
                    BRepBndLib::Add(shape, bbox);
                    Standard_Real xMin{}, yMin{}, zMin{}, xMax{}, yMax{}, zMax{};
                    bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
                    return gp_Pnt((xMin + xMax) / 2.0, (yMin + yMax) / 2.0, (zMin + zMax) / 2.0);
                };

            std::vector<TopoDS_Shape> explodedShapes;

            gp_Pnt globalCenter(0, 0, 0);
            if (!objectList.empty())
            {
                Bnd_Box globalBox;
                for (const auto& object : objectList)
                {
                    const auto aisShape = Handle(AIS_Shape)::DownCast(object);
                    const auto& shape = aisShape->Shape();
                    BRepBndLib::Add(shape, globalBox);
                }
                Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
                globalBox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
                globalCenter = gp_Pnt((xMin + xMax) / 2.0, (yMin + yMax) / 2.0, (zMin + zMax) / 2.0);
            }

            for (const auto& object : objectList)
            {
                const auto aisShape = Handle(AIS_Shape)::DownCast(object);
                const auto& shape = aisShape->Shape();
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
    for (const auto& shape : explodedShapes)
    {
        Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
        aisShape->SetDisplayMode(AIS_Shaded);
        aisShape->SetColor(Quantity_NOC_SKYBLUE);
        m_occView->setShape( aisShape );
    }
    m_occView->reDraw();
}
