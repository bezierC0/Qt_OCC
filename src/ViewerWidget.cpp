#include "ViewerWidget.h"
#include "MainWindow.h"
#include "WidgetModelTree.h"
#include "OCCView.h"
#include "Tree.h"
#include "ViewManager.h"


#include <QtWidgets/QVBoxLayout> // Corrected path
#include <QMessageBox>
#include <QCoreApplication>

/* read */
#include <STEPControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TopExp_Explorer.hxx>
#include <TDocStd_Document.hxx>
/* boolean */
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
/* builder */
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <GC_MakeArcOfCircle.hxx>

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
#include <Graphic3d_ClipPlane.hxx> // Added missing include for clipping
#include <gp_Pln.hxx>              // Added missing include for clipping
#include <Quantity_Color.hxx>      // Added missing include for colors
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
/* geomtry */
#include <Geom_BezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Plane.hxx>
#include <Geom_Surface.hxx>
#include <Geom_Line.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <GeomLib_IsPlanarSurface.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>

namespace
{
    bool isShapeAssembly(const TDF_Label &lbl)
    {
        return XCAFDoc_ShapeTool::IsAssembly(lbl);
    }

    bool isShapeReference(const TDF_Label &lbl)
    {
        return XCAFDoc_ShapeTool::IsReference(lbl);
    }

    bool isShapeSimple(const TDF_Label &lbl)
    {
        return XCAFDoc_ShapeTool::IsSimpleShape(lbl);
    }

    bool isShapeComponent(const TDF_Label &lbl)
    {
        return XCAFDoc_ShapeTool::IsComponent(lbl);
    }

    bool isShapeCompound(const TDF_Label &lbl)
    {
        return XCAFDoc_ShapeTool::IsCompound(lbl);
    }

    bool isShapeSub(const TDF_Label &lbl)
    {
        return XCAFDoc_ShapeTool::IsSubShape(lbl);
    }
    bool isShape(const TDF_Label &lbl)
    {
        return XCAFDoc_ShapeTool::IsShape(lbl);
    }

    TDF_LabelSequence shapeComponents(const TDF_Label &lbl)
    {
        TDF_LabelSequence seq;
        XCAFDoc_ShapeTool::GetComponents(lbl, seq);
        return seq;
    }

    TDF_Label shapeReferred(const TDF_Label &lbl)
    {
        TDF_Label referred;
        XCAFDoc_ShapeTool::GetReferredShape(lbl, referred);
        return referred;
    }

    // mayo xcaf.cpp deepBuildAssemblyTree
    uint32_t deepBuildAssemblyTree(uint32_t parentNode, const TDF_Label &label, Tree<TDF_Label> &modelTree)
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
            // if (isShape(label))
            //{
            //     // GraphicsObjectPtr GraphicsShapeObjectDriver::createObject(const TDF_Label& label) const
            //     std::cout << " isShape : \n";
            // }
            for (const TDF_Label &child : shapeComponents(label))
                deepBuildAssemblyTree(node, child, modelTree);
        }
        else if (isShapeReference(label))
        {
            // Handle_TDataStd_Name attrName;
            // if (isShape(label))
            //{
            //     // GraphicsObjectPtr GraphicsShapeObjectDriver::createObject(const TDF_Label& label) const
            //     std::cout << " isShape : \n";
            // }

            const TDF_Label referred = shapeReferred(label);
            deepBuildAssemblyTree(node, referred, modelTree);
        }

        return node;
    }
}

ViewerWidget::ViewerWidget(QWidget *parent) : QWidget(parent)
{
    m_occView = new OCCView(this);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_occView);
    layout->setMargin(0);

    m_doc = std::make_shared<Document>();
    ViewManager::getInstance().addView(m_occView);
}

ViewerWidget::~ViewerWidget()
{
    if (m_occView)
    {
        delete m_occView;
        m_occView = nullptr;
    }
}

void ViewerWidget::clearAll()
{
    if (m_doc)
    {
        m_doc->m_list.clear();
    }

    if (m_occView)
    {
        m_occView->clearShape();
        m_occView->clearSelectedObjects();
        m_occView->reDraw();
    }

    const MainWindow* mainWindow = qobject_cast<MainWindow*>(this->window());
    if (mainWindow)
    {
        ModelTreeWidget* treeWidget = mainWindow->GetModelTreeWidget();
        if (treeWidget)
        {
            Tree<TDF_Label> emptyTree;
            treeWidget->setModelTree(emptyTree);
        }
    }
}

void ViewerWidget::loadModel(const QString &filename) 
{
    clearAll(); 
    Tree<TDF_Label> modelTree; 
    if (filename.endsWith(".step") || filename.endsWith(".stp"))
    {
        STEPCAFControl_Reader reader;
        IFSelect_ReturnStatus status = reader.ReadFile(filename.toStdString().c_str());
        if (status != IFSelect_RetDone)
        {
            QMessageBox::critical(nullptr, QCoreApplication::translate("ViewerWidget", "Error"), QCoreApplication::translate("ViewerWidget", "STEP file read failed"));
        }
        Handle(TDocStd_Document) doc;
        Handle(XCAFDoc_ShapeTool) shapeTool;
        Handle(XCAFDoc_ColorTool) colorTool;

        Handle(XCAFApp_Application)::DownCast(XCAFApp_Application::GetApplication())->NewDocument("BinXCAF", doc);

        if (!reader.Transfer(doc))
        {
            QMessageBox::critical(nullptr, QCoreApplication::translate("ViewerWidget", "Error"), QCoreApplication::translate("ViewerWidget", "STEP transfer to document failed"));
            return;
        }

        shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
        colorTool = XCAFDoc_DocumentTool::ColorTool(doc->Main());

        int nbRoots = reader.NbRootsForTransfer();

        TDF_LabelSequence labels;
        shapeTool->GetFreeShapes(labels);
        for (const TDF_Label &label : labels)
        {
            deepBuildAssemblyTree(0, label, modelTree);
        }

        for (const auto &it : modelTree.m_vecNode)
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
                    new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 1.));

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

    for (const auto &aisObject : m_doc->m_list)
    {
        m_occView->setShape(aisObject);
    }

    const MainWindow *mainWindow = qobject_cast<MainWindow *>(this->window());
    auto treeWidget = mainWindow->GetModelTreeWidget();
    treeWidget->setModelTree(modelTree);
    m_occView->reDraw();
}

void ViewerWidget::viewFit()
{
    m_occView->viewfit();
}

void ViewerWidget::viewIsometric() const
{
    m_occView->viewIsometric();
}

void ViewerWidget::viewTop() const
{
    m_occView->viewTop();
}

void ViewerWidget::viewBottom() const
{
    m_occView->viewBottom();
}

void ViewerWidget::viewLeft() const
{
    m_occView->viewLeft();
}

void ViewerWidget::viewRight() const
{
    m_occView->viewRight();
}

void ViewerWidget::viewFront() const
{
    m_occView->viewFront();
}

void ViewerWidget::viewBack() const
{
    m_occView->viewBack();
}

void ViewerWidget::setDisplayMode(const int mode)
{
    m_occView->setDisplayMode(static_cast<View::DisplayMode>(mode));
}

void ViewerWidget::switchSelect(bool checked)
{
    m_occView->setMouseMode( checked ? View::MouseMode::SELECTION : View::MouseMode::NONE );
}

void ViewerWidget::setFilters(const std::map<TopAbs_ShapeEnum, bool>& filters)
{
    // Ensure selection mode is enabled if any filters are active
    bool anyFilterActive = false;
    for (const auto& pair : filters) {
        if (pair.second) {
            anyFilterActive = true;
            break;
        }
    }
    m_occView->setMouseMode(anyFilterActive ? View::MouseMode::SELECTION : View::MouseMode::NONE);

    for (const auto& pair : filters) {
        m_occView->updateSelectionFilter(pair.first, pair.second);
    }
}

void ViewerWidget::updateSelectionFilter(TopAbs_ShapeEnum filter, bool isActive)
{
    m_occView->updateSelectionFilter(filter, isActive);

    // When a filter changes, we might need to update the mouse selection mode.
    // If any filter is active, selection mode should be on. Otherwise, it should be off.
    const auto& filters = m_occView->getSelectionFilters();
    bool anyFilterActive = false;
    for (const auto& pair : filters) {
        if (pair.second) {
            anyFilterActive = true;
            break;
        }
    }
    m_occView->setMouseMode(anyFilterActive ? View::MouseMode::SELECTION : View::MouseMode::NONE);
}

void ViewerWidget::createWorkPlane()
{
    const gp_Dir normal(0.0, 0.0, 1.0);
    const gp_Pnt point(0.0, 0.0, 10.0);
}

void ViewerWidget::transform()
{
    m_occView->transform();
}

void ViewerWidget::checkInterference()
{
    m_occView->checkInterference();
}

void ViewerWidget::clipping(const gp_Dir &normal, const gp_Pnt &point, const bool isOn)
{
    m_occView->addClippingPlane( point,normal );
}

void ViewerWidget::explosion()
{
    m_occView->checkInterference();
}

void ViewerWidget::measureDistance()
{
    const auto &selectedList = m_occView->getSelectedObjects();
    if (selectedList.size() != 2)
    {
        QMessageBox::warning(this, QCoreApplication::translate("ViewerWidget", "Selection Error"), QCoreApplication::translate("ViewerWidget", "Please select exactly two vertices."));
        return;
    }

    const auto interactiveObject0 = selectedList.at(0);
    const auto interactiveObject1 = selectedList.at(1);
    if (interactiveObject0.IsNull() || interactiveObject1.IsNull())
        return;

    const auto aisShape0 = Handle(AIS_Shape)::DownCast(interactiveObject0);
    const auto aisShape1 = Handle(AIS_Shape)::DownCast(interactiveObject1);
    if (aisShape0.IsNull() || aisShape1.IsNull())
        return;

    const TopoDS_Shape &shape0 = aisShape0->Shape();
    const TopoDS_Shape &shape1 = aisShape1->Shape();
    if (shape0.IsNull() || shape1.IsNull() ||
        shape0.ShapeType() != TopAbs_VERTEX || shape1.ShapeType() != TopAbs_VERTEX)
    {
        QMessageBox::warning(this, QCoreApplication::translate("ViewerWidget", "Selection Error"), QCoreApplication::translate("ViewerWidget", "Please select exactly two vertices."));
        return;
    }

    const TopoDS_Vertex &vertex0 = TopoDS::Vertex(shape0);
    const TopoDS_Vertex &vertex1 = TopoDS::Vertex(shape1);

    const auto point0 = BRep_Tool::Pnt(vertex0);
    const auto point1 = BRep_Tool::Pnt(vertex1);

    const Standard_Real distance = point0.Distance(point1);
    QMessageBox::information(this, QCoreApplication::translate("ViewerWidget", "Distance"),
                             QCoreApplication::translate("ViewerWidget", "Distance between the two vertices: %1").arg(distance));
}

void ViewerWidget::createPoint()
{
    gp_Pnt p(10, 20, 30);
    BRepBuilderAPI_MakeVertex vertexMaker(p);
    if (vertexMaker.IsDone())
    {
        displayShape(vertexMaker.Shape(), 1.0, 1.0, 1.0); 
    }
}

void ViewerWidget::createLine()
{
    gp_Pnt p1(0, 0, 0);
    gp_Pnt p2(50, 50, 50);
    BRepBuilderAPI_MakeEdge edge(p1, p2);
    if (edge.IsDone())
    {
        displayShape(edge.Shape(), 1.0, 0.0, 0.0);
    }
}

void ViewerWidget::createRectangle()
{
    gp_Pnt p1(0, 0, 0);
    gp_Pnt p2(40, 0, 0);
    gp_Pnt p3(40, 30, 0);
    gp_Pnt p4(0, 30, 0);
    BRepBuilderAPI_MakePolygon poly;
    poly.Add(p1);
    poly.Add(p2);
    poly.Add(p3);
    poly.Add(p4);
    poly.Add(p1); // Close the polygon
    if (poly.IsDone())
    {
        BRepBuilderAPI_MakeFace face(poly.Wire());
        if (face.IsDone())
        {
            displayShape(face.Shape(), 0.0, 1.0, 0.0);
        }
    }
}

void ViewerWidget::createCircle()
{
    gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); // Z axis
    gp_Circ circle(axis, 25.0);                   // Radius 25
    BRepBuilderAPI_MakeEdge edge(circle);
    if (edge.IsDone())
    {
        displayShape(edge.Shape(), 0.0, 0.0, 1.0);
    }
}

void ViewerWidget::createArc()
{
    gp_Pnt center(0, 0, 0);
    gp_Pnt p1(30, 0, 0);
    gp_Pnt p2(0, 30, 0);
    GC_MakeArcOfCircle arc(center, p1, p2);
    if (arc.IsDone())
    {
        BRepBuilderAPI_MakeEdge edge(arc.Value());
        if (edge.IsDone())
        {
            displayShape(edge.Shape(), 1.0, 1.0, 0.0);
        }
    }
}

void ViewerWidget::createEllipse()
{
    gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); // Z axis
    gp_Elips ellipse(axis, 30.0, 15.0);           // Major radius 30, minor 15
    BRepBuilderAPI_MakeEdge edge(ellipse);
    if (edge.IsDone())
    {
        displayShape(edge.Shape(), 1.0, 0.0, 1.0);
    }
}

void ViewerWidget::createPolygon()
{
    BRepBuilderAPI_MakePolygon poly;
    poly.Add(gp_Pnt(0, 0, 0));
    poly.Add(gp_Pnt(20, 10, 0));
    poly.Add(gp_Pnt(30, 30, 0));
    poly.Add(gp_Pnt(10, 40, 0));
    poly.Add(gp_Pnt(-10, 20, 0));
    poly.Add(gp_Pnt(0, 0, 0)); // Close it
    if (poly.IsDone())
    {
        displayShape(poly.Wire(), 0.0, 1.0, 1.0);
    }
}

void ViewerWidget::createBezierCurve()
{
    TColgp_Array1OfPnt poles(1, 4);
    poles.SetValue(1, gp_Pnt(0, 0, 0));
    poles.SetValue(2, gp_Pnt(10, 40, 0));
    poles.SetValue(3, gp_Pnt(40, 40, 0));
    poles.SetValue(4, gp_Pnt(50, 0, 0));
    Handle(Geom_BezierCurve) bezier = new Geom_BezierCurve(poles);
    BRepBuilderAPI_MakeEdge edge(bezier);
    if (edge.IsDone())
    {
        displayShape(edge.Shape(), 0.5, 0.5, 0.5);
    }
}

void ViewerWidget::createNurbsCurve()
{
    TColgp_Array1OfPnt poles(1, 4);
    poles.SetValue(1, gp_Pnt(0, 0, 0));
    poles.SetValue(2, gp_Pnt(10, 40, 0));
    poles.SetValue(3, gp_Pnt(40, -40, 0));
    poles.SetValue(4, gp_Pnt(50, 0, 0));

    TColStd_Array1OfReal knots(1, 2);
    knots.SetValue(1, 0.0);
    knots.SetValue(2, 1.0);

    TColStd_Array1OfInteger mults(1, 2);
    mults.SetValue(1, 4);
    mults.SetValue(2, 4);

    Standard_Integer degree = 3;

    Handle(Geom_BSplineCurve) bspline = new Geom_BSplineCurve(poles, knots, mults, degree);
    BRepBuilderAPI_MakeEdge edge(bspline);
    if (edge.IsDone())
    {
        displayShape(edge.Shape(), 0.2, 0.8, 0.2);
    }
}

void ViewerWidget::createBox()
{
    const gp_Pnt P1{-10,-20,-30};
    const gp_Pnt P2{30,20,15};
    BRepPrimAPI_MakeBox box(P1,P2);
    displayShape(box.Shape(), 1.0, 0.0, 1.0);
}

void ViewerWidget::createPyramid()
{
}

void ViewerWidget::createSphere()
{
    BRepPrimAPI_MakeSphere sphere( gp_Pnt( 0, 0, 0 ), 5.0 ) ;
    displayShape( sphere.Shape(), 1.0, 0.0, 0.0 ) ;  // NOLINT
}

void ViewerWidget::createCylinder()
{
    BRepPrimAPI_MakeCylinder cylinder(5.0, 10.0);
    displayShape(cylinder.Shape(), 0.0, 1.0, 0.0);
}

void ViewerWidget::createCone()
{
    BRepPrimAPI_MakeCone cone(5.0, 0.0, 10.0);
    displayShape(cone.Shape(), 0.0, 1.0, 1.0);
}

void ViewerWidget::booleanUnion()
{
    TopoDS_Shape shapeA;
    TopoDS_Shape shapeB;
    if( !getBooleanTargets(shapeA, shapeB) )
        return ;

    BRepAlgoAPI_Fuse booleanFun(shapeA, shapeB);
    if ( booleanFun.IsDone()) {
        TopoDS_Shape result = booleanFun.Shape();
        displayShape(result, 1.0, 0.0, 1.0); 
        m_occView->clearSelectedObjects(); 
    } else {
        QMessageBox::warning(this, tr("Boolean Operation Failed"), tr("Boolean Union operation failed!"));
    }
}

void ViewerWidget::booleanIntersection()
{
    TopoDS_Shape shape1;
    TopoDS_Shape shape2;
    if( !getBooleanTargets(shape1, shape2) )
        return ;
    BRepAlgoAPI_Common booleanFun(shape1, shape2);
    if ( booleanFun.IsDone()) {
        TopoDS_Shape result = booleanFun.Shape();
        displayShape(result, 0.0, 1.0, 0.0); 
        m_occView->clearSelectedObjects();
    }  else {
        QMessageBox::warning(this, tr("Boolean Operation Failed"), tr("Boolean Intersection operation failed!"));
    }
}

void ViewerWidget::booleanDifference()
{
    TopoDS_Shape shape1;
    TopoDS_Shape shape2;
    if( !getBooleanTargets(shape1, shape2) )
        return ;
    BRepAlgoAPI_Cut booleanFun(shape1, shape2);
    if ( booleanFun.IsDone()) {
        TopoDS_Shape result = booleanFun.Shape();
        displayShape(result, 0.0, 0.0, 1.0); 
        m_occView->clearSelectedObjects();
    } else {
        QMessageBox::warning(this, tr("Boolean Operation Failed"), tr("Boolean Difference operation failed!"));
    }
}

void ViewerWidget::patternLinear()
{
    auto selectedObjects = m_occView->getSelectedObjects();
    if ( selectedObjects.size() != 2 )
    {
        QMessageBox::warning(this, QCoreApplication::translate("ViewerWidget", "Selection Error"), QCoreApplication::translate("ViewerWidget", "Please select exactly two shapes."));
        return ;
    }
    auto interactiveObject0 = selectedObjects.at(0);
    auto interactiveObject1 = selectedObjects.at(1);
    if ( interactiveObject0.IsNull() || interactiveObject1.IsNull() )
    {
        return ;
    }

    const auto aisShape0 = Handle(AIS_Shape)::DownCast(interactiveObject0);
    const auto aisShape1 = Handle(AIS_Shape)::DownCast(interactiveObject1);

    if ( aisShape0.IsNull() || aisShape1.IsNull() )
    {
        return ;
    }

    const auto shape0 = aisShape0->Shape();
    const auto shape1 = aisShape1->Shape();

    if ( shape0.IsNull() || shape1.IsNull() )
    {
        return ;
    }
    
    if ( shape0.ShapeType() != TopAbs_SHAPE && shape1.ShapeType() != TopAbs_EDGE ){
        return;
    }

    Standard_Real first, last;
    auto geomLine = Handle(Geom_Line)::DownCast(BRep_Tool::Curve(TopoDS::Edge(shape1),first,last));
    if (geomLine.IsNull()) {
        return;
    }

    const TopoDS_Shape baseShape = shape0;
    const gp_Vec direction = geomLine->Position().Direction();
    Standard_Real spacing = 10;
    Standard_Integer count = 3;

    m_occView->patternLinear(baseShape, direction, spacing, count);
}

void ViewerWidget::patternCircular()
{
    auto selectedObjects = m_occView->getSelectedObjects();
    if ( selectedObjects.size() != 2 )
    {
        QMessageBox::warning(this, QCoreApplication::translate("ViewerWidget", "Selection Error"), QCoreApplication::translate("ViewerWidget", "Please select exactly two shapes."));
        return ;
    }
    auto interactiveObject0 = selectedObjects.at(0);
    auto interactiveObject1 = selectedObjects.at(1);
    if ( interactiveObject0.IsNull() || interactiveObject1.IsNull() )
    {
        return ;
    }

    const auto aisShape0 = Handle(AIS_Shape)::DownCast(interactiveObject0);
    const auto aisShape1 = Handle(AIS_Shape)::DownCast(interactiveObject1);

    if ( aisShape0.IsNull() || aisShape1.IsNull() )
    {
        return ;
    }

    const auto& shape0 = aisShape0->Shape();
    const auto& shape1 = aisShape1->Shape();

    if ( shape0.IsNull() || shape1.IsNull() )
    {
        return ;
    }

    if ( shape0.ShapeType() != TopAbs_SHAPE && shape1.ShapeType() != TopAbs_EDGE ){
        return;
    }

    Standard_Real first, last;
    auto geomCircle = Handle(Geom_Circle)::DownCast(BRep_Tool::Curve(TopoDS::Edge(shape1),first,last));
    if (geomCircle.IsNull()) {
        return;
    }

    const TopoDS_Shape baseShape = shape0;
    const gp_Ax1 axis = geomCircle->Axis();
    Standard_Real angleStep{ 10.0 };
    Standard_Integer count{ 3 };
    m_occView->patternCircular(baseShape, axis, angleStep, count);
}

void ViewerWidget::mirrorByPlane()
{
    auto selectedObjects = m_occView->getSelectedObjects();
    if ( selectedObjects.size() != 2 )
    {
        QMessageBox::warning(this, QCoreApplication::translate("ViewerWidget", "Selection Error"), QCoreApplication::translate("ViewerWidget", "Please select exactly two shapes."));
        return ;
    }
    auto interactiveObjectShape = selectedObjects.at(0);
    auto interactiveObjectPlane = selectedObjects.at(1);
    if ( interactiveObjectShape.IsNull() || interactiveObjectPlane.IsNull() )
    {
        QMessageBox::warning(this, QCoreApplication::translate("ViewerWidget", "Selection Error"), QCoreApplication::translate("ViewerWidget", "Interactive object is null."));
        return ;
    }

    const auto aisShape = Handle(AIS_Shape)::DownCast(interactiveObjectShape);
    const auto aisPlane = Handle(AIS_Shape)::DownCast(interactiveObjectPlane);

    if ( aisShape.IsNull() || aisPlane.IsNull() )
    {
        QMessageBox::warning(this, QCoreApplication::translate("ViewerWidget", "Selection Error"), QCoreApplication::translate("ViewerWidget", "AisShape is null."));
        return ;
    }

    const auto shape = aisShape->Shape();
    const auto plane = aisPlane->Shape();

    if ( shape.IsNull() || plane.IsNull() )
    {
        return ;
    }

    if ( shape.ShapeType() != TopAbs_SHAPE && plane.ShapeType() != TopAbs_FACE ){
        return ;
    } 

    TopoDS_Face face = TopoDS::Face(plane);
    Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
    Handle(Geom_Plane) geomPlane = Handle(Geom_Plane)::DownCast(surface);

    if (!geomPlane.IsNull()) {
        return;
    }
    m_occView->mirrorByPlane(shape, geomPlane->Pln());
}

void ViewerWidget::mirrorByAxis()
{
    auto selectedObjects = m_occView->getSelectedObjects();
    if ( selectedObjects.size() != 2 )
    {
        QMessageBox::warning(this, QCoreApplication::translate("ViewerWidget", "Selection Error"), QCoreApplication::translate("ViewerWidget", "Please select exactly two shapes."));
        return ;
    }
    auto interactiveObject0 = selectedObjects.at(0);
    auto interactiveObject1 = selectedObjects.at(1);
    if ( interactiveObject0.IsNull() || interactiveObject1.IsNull() )
    {
        QMessageBox::warning(this, QCoreApplication::translate("ViewerWidget", "Selection Error"), QCoreApplication::translate("ViewerWidget", "Interactive object is null."));
        return ;
    }

    const auto aisShape0 = Handle(AIS_Shape)::DownCast(interactiveObject0);
    const auto aisShape1 = Handle(AIS_Shape)::DownCast(interactiveObject1);

    if ( aisShape0.IsNull() || aisShape1.IsNull() )
    {
        QMessageBox::warning(this, QCoreApplication::translate("ViewerWidget", "Selection Error"), QCoreApplication::translate("ViewerWidget", "AisShape is null."));
        return ;
    }

    const auto shape0 = aisShape0->Shape();
    const auto shape1 = aisShape1->Shape();

    if ( shape0.IsNull() || shape1.IsNull() )
    {
        return ;
    }

    if ( shape0.ShapeType() != TopAbs_SHAPE && shape1.ShapeType() != TopAbs_EDGE ){
        return;
    }

    TopoDS_Edge edge = TopoDS::Edge(shape1);
    Standard_Real first, last;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

    if (curve.IsNull()) {
        return;
    }

    Handle(Geom_Curve) geomCurve = Handle(Geom_Curve)::DownCast(curve);
    Handle(Geom_Line) geomLine = Handle(Geom_Line)::DownCast(geomCurve);

    if (geomLine.IsNull()) {
        return;
    }

    const gp_Ax1 mirrorAxis = geomLine->Position();
    m_occView->mirrorByAxis(shape0, mirrorAxis);
}

void ViewerWidget::displayShape(const TopoDS_Shape &shape, const double r, const double g, const double b)
{
    Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
    aisShape->SetDisplayMode(AIS_Shaded);
    aisShape->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
    m_occView->setShape(aisShape);
    m_occView->reDraw();
    m_occView->viewfit(); // Fit view to the new shape
}

void ViewerWidget::removeShape(const TopoDS_Shape &shape)
{
    // TODO
}

const std::map<TopAbs_ShapeEnum, bool>& ViewerWidget::getSelectionFilters() const {
    const auto acitveView = ViewManager::getInstance().getActiveView() ;
    assert(acitveView);
    return acitveView->getSelectionFilters() ;
}

bool ViewerWidget::getBooleanTargets(TopoDS_Shape &target1, TopoDS_Shape &target2)
{
    const auto acitveView = ViewManager::getInstance().getActiveView();
    if( !acitveView )
        return false ;
    const auto& selectedList = acitveView->getSelectedAisShape(2);
    if (selectedList.size() != 2)
        return false ;

    const auto aisShapeA = selectedList.at(0);
    const auto aisshapeB = selectedList.at(1);
    if( aisShapeA.IsNull() || aisshapeB.IsNull() )
        return false ;

    const auto shapeA = aisShapeA->Shape();
    const auto shapeB = aisshapeB->Shape();
    if( shapeA.IsNull() || shapeB.IsNull() )
        return false ;
    target1 = shapeA ;
    target2 = shapeB ;
    return true ;
}
