#include "ViewerWidget.h"
#include "MainWindow.h"
#include "WidgetModelTree.h"
#include "SelectedEntity.h"
#include "OCCView.h"
#include "Tree.h"
#include "ViewManager.h"

#include "ui/DialogCreatePoint.h"
#include "ui/DialogCreateBox.h"
#include "ui/DialogCreateLine.h"
#include "ui/DialogCreateRectangle.h"
#include "ui/DialogCreateCircle.h"
#include <QtWidgets/QVBoxLayout> // Corrected path
#include <QMessageBox>
#include <QCoreApplication>
#include <QDebug>

/* read */
#include <STEPControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TopExp_Explorer.hxx>
#include <TDocStd_Document.hxx>
#include <TopAbs_ShapeEnum.hxx>
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
#include <TopoDS_Compound.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFPrs_AISObject.hxx>
#include <TDataStd_Name.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <AIS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>
#include <Graphic3d_ClipPlane.hxx> // Added missing include for clipping
#include <gp_Pln.hxx>              // Added missing include for clipping
#include <Quantity_Color.hxx>      // Added missing include for colors
#include <GProp_GProps.hxx>
#include <TDF_ChildIterator.hxx>

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
#include <TopTools_ListOfShape.hxx>

/* healing */
#include <ShapeFix_Shape.hxx>
#include <STEPControl_Writer.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_Solid.hxx>
#include <ShapeFix_Face.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeFix_FixSmallFace.hxx>
#include <ShapeFix.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <ShapeBuild_ReShape.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>


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

#include <TDF_ChildIterator.hxx>

// Function to recursively inspect labels and highlight the matching one
bool highlightLabelRecursively(const TDF_Label& targetLabel, const TDF_Label& currentLabel, Handle(AIS_InteractiveContext) context, Handle(XCAFPrs_AISObject) xcafObj)
{
    if (currentLabel.IsEqual(targetLabel)) {
        if (!context.IsNull() && !xcafObj.IsNull()) {
             // For XCAFPrs_AISObject, selection often works at the main object level or via selection modes.
             // Simplest approach: Highlight the whole XCAF object if it contains this label.
             context->SetSelected(xcafObj, true);
        }
        return true;
    }

    // Check children
    TDF_ChildIterator it(currentLabel);
    for (; it.More(); it.Next()) {
        if (highlightLabelRecursively(targetLabel, it.Value(), context, xcafObj))
            return true;
    }
    
    // Also check components/referred shapes if valid
    if (isShapeAssembly(currentLabel)) {
         TDF_LabelSequence seq = shapeComponents(currentLabel);
         for (Standard_Integer i = 1; i <= seq.Length(); ++i) {
             if (highlightLabelRecursively(targetLabel, seq.Value(i), context, xcafObj))
                 return true;
         }
    }
    
    return false;
}


// Helper to determine shape type string
std::string getShapeTypeString(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) return "Shape";
    switch (shape.ShapeType())
    {
        case TopAbs_COMPOUND:       return "Compound";
        case TopAbs_COMPSOLID:      return "CompSolid";
        case TopAbs_SOLID:          return "Solid";
        case TopAbs_SHELL:          return "Shell";
        case TopAbs_FACE:           return "Face";
        case TopAbs_WIRE:           return "Wire";
        case TopAbs_EDGE:           return "Edge";
        case TopAbs_VERTEX:         return "Vertex";
        case TopAbs_SHAPE:          return "Shape";
        default:                    return "Unknown";
    }
}


// mayo xcaf.cpp deepBuildAssemblyTree
uint32_t deepBuildAssemblyTree(uint32_t parentNode, const TDF_Label &label,
                               Tree<TDF_Label> &modelTree)
{

    const TreeNodeId node = modelTree.appendChild(parentNode, label);
    // GraphicsObjectPtr GraphicsShapeObjectDriver::createObject(const TDF_Label& label) const
    //#define mydebug
    #ifdef mydebug
    Handle_TDataStd_Name attrName;
    if (label.FindAttribute(TDataStd_Name::GetID(), attrName)) {
        std::cout << node << "  : " << attrName->Get() << std::endl;
    }
    #endif
    if (isShapeAssembly(label)) {
        #ifdef mydebug
        if (isShape(label))
        {
            std::cout << "isshape" << std::endl;
            // GraphicsObjectPtr GraphicsShapeObjectDriver::createObject(const TDF_Label& label) const
            if (label.FindAttribute(TDataStd_Name::GetID(), attrName)) {
                std::cout << node << "  : " << attrName->Get() << std::endl;
            }
        }
        #endif
        for (const TDF_Label &child : shapeComponents(label))
        {
            deepBuildAssemblyTree(node, child, modelTree);
        }
    } else if (isShapeReference(label)) {
        #ifdef mydebug
        Handle_TDataStd_Name attrName;
        if (isShape(label))
        {
            // GraphicsObjectPtr GraphicsShapeObjectDriver::createObject(const TDF_Label& label) const
            std::cout << "isshape" << std::endl;
            if (label.FindAttribute(TDataStd_Name::GetID(), attrName)) {
                std::cout << node << "  : " << attrName->Get() << std::endl;
            }
        }
        #endif
        const TDF_Label referred = shapeReferred(label);
        deepBuildAssemblyTree(node, referred, modelTree);
    }

    return node;
}
} // namespace

struct ViewerWidget::Document {
    std::vector<opencascade::handle<AIS_InteractiveObject>> m_list;
    Handle(TDocStd_Document) m_ocafDoc;
};

ViewerWidget::ViewerWidget(QWidget *parent) : QWidget(parent)
{
    m_occView = new OCCView(this);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_occView);
    layout->setMargin(0);

    m_doc = std::make_shared<Document>();
    
    // Initialize OCAF Application and Document
    Handle(XCAFApp_Application) app = Handle(XCAFApp_Application)::DownCast(XCAFApp_Application::GetApplication());
    app->NewDocument("BinXCAF", m_doc->m_ocafDoc);

    ViewManager::getInstance().addView(m_occView);
}

ViewerWidget::~ViewerWidget()
{
    if (m_occView) {
        delete m_occView;
        m_occView = nullptr;
    }
}

void ViewerWidget::clearAll()
{
    if (m_doc) {
        m_doc->m_list.clear();
    }

    // Reset OCAF Document
    if (!m_doc->m_ocafDoc.IsNull()) {
        Handle(XCAFApp_Application) app = Handle(XCAFApp_Application)::DownCast(XCAFApp_Application::GetApplication());
        app->NewDocument("BinXCAF", m_doc->m_ocafDoc);
    }

    if (m_occView) {
        m_occView->clearShape();
        m_occView->clearSelectedObjects();
        m_occView->reDraw();
    }

    const MainWindow *mainWindow = qobject_cast<MainWindow *>(this->window());
    if (mainWindow) {
        ModelTreeWidget *treeWidget = mainWindow->GetModelTreeWidget();
        if (treeWidget) {
            Tree<TDF_Label> emptyTree;
            treeWidget->setModelTree(emptyTree);
        }
    }
}

void ViewerWidget::updateTree()
{
    if (m_doc->m_ocafDoc.IsNull()) return;

    Tree<TDF_Label> modelTree;
    Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(m_doc->m_ocafDoc->Main());
    
    TDF_LabelSequence labels;
    shapeTool->GetFreeShapes(labels);
    for (Standard_Integer i = 1; i <= labels.Length(); ++i) {
        deepBuildAssemblyTree(0, labels.Value(i), modelTree);
    }

    const MainWindow *mainWindow = qobject_cast<MainWindow *>(this->window());
    if (mainWindow) {
        auto treeWidget = mainWindow->GetModelTreeWidget();
        if (treeWidget) {
            treeWidget->setModelTree(modelTree);
        }
    }
}


void ViewerWidget::loadModel(const QString &filename)
{
    clearAll();
    Tree<TDF_Label> modelTree;
    if (filename.endsWith(".step") || filename.endsWith(".stp")) {
        STEPCAFControl_Reader reader;
        IFSelect_ReturnStatus status = reader.ReadFile(filename.toStdString().c_str());
        if (status != IFSelect_RetDone) {
            QMessageBox::critical(
                nullptr, QCoreApplication::translate("ViewerWidget", "Error"),
                QCoreApplication::translate("ViewerWidget", "STEP file read failed"));
        }
        
        // Use member m_ocafDoc
        if (!reader.Transfer(m_doc->m_ocafDoc)) {
            QMessageBox::critical(
                nullptr, QCoreApplication::translate("ViewerWidget", "Error"),
                QCoreApplication::translate("ViewerWidget", "STEP transfer to document failed"));
            return;
        }

        Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(m_doc->m_ocafDoc->Main());
        
        TDF_LabelSequence labels;
        shapeTool->GetFreeShapes(labels);
        // Build Tree (will be also done in updateTree, but we need labels here for display)
        for (Standard_Integer i = 1; i <= labels.Length(); ++i) {
             // deepBuildAssemblyTree(0, labels.Value(i), modelTree); // updateTree will do this
        }

        // Collect shapes for healing
        if (m_importWithHealing) {
            TopoDS_Compound compound;
            BRep_Builder builder;
            builder.MakeCompound(compound);
            bool hasShapes = false;
            for (Standard_Integer i = 1; i <= labels.Length(); ++i) {
                const TDF_Label& label = labels.Value(i);
                TopoDS_Shape shape;
                if (XCAFDoc_ShapeTool::GetShape(label, shape)) {
                    builder.Add(compound, shape);
                    hasShapes = true;
                }
            }

            if (hasShapes) {
                repairAndSave(compound);
            }
        }

        // Display Shapes
        for (Standard_Integer i = 1; i <= labels.Length(); ++i) {
            TDF_Label label = labels.Value(i);
            
            // Better to just create XCAFPrs object for top level shapes
            Handle(XCAFPrs_AISObject) object = new XCAFPrs_AISObject(label);
            object->SetDisplayMode(AIS_Shaded);
            //object->SetMaterial(Graphic3d_NOM_PLASTER); // Set Material
            object->Attributes()->SetFaceBoundaryDraw(true);
            object->Attributes()->SetFaceBoundaryAspect(new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 1.));

            m_doc->m_list.emplace_back(object);
        }
    } else if (filename.endsWith(".iges") || filename.endsWith(".igs")) {
        IGESControl_Reader reader;
        if (reader.ReadFile(filename.toStdString().c_str()) == IFSelect_RetDone) {
            reader.TransferRoots();
            int nbRoots = reader.NbRootsForTransfer();
            
            Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(m_doc->m_ocafDoc->Main());
            Handle(XCAFDoc_ColorTool) colorTool = XCAFDoc_DocumentTool::ColorTool(m_doc->m_ocafDoc->Main());

            for (int i = 1; i <= nbRoots; ++i) {
                TopoDS_Shape subShape = reader.Shape(i);
                if (subShape.IsNull())
                    continue;
                
                // Add to OCAF
                TDF_Label label = shapeTool->AddShape(subShape, false);
                TDataStd_Name::Set(label, "Imported IGES Shape");

                // Display using OCAF aware object or simple AIS
                Handle(XCAFPrs_AISObject) object = new XCAFPrs_AISObject(label);
                object->SetDisplayMode(AIS_Shaded);
                object->SetColor(Quantity_NOC_WHITE);
                m_doc->m_list.emplace_back(object);
            }
        }
    }
    
    updateTree();

    for (const auto &aisObject : m_doc->m_list) {
        m_occView->setShape(aisObject);
    }

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
    m_occView->setMouseMode(checked ? View::MouseMode::SELECTION : View::MouseMode::NONE);
}

void ViewerWidget::setFilters(const std::map<TopAbs_ShapeEnum, bool> &filters)
{
    // Ensure selection mode is enabled if any filters are active
    bool anyFilterActive = false;
    for (const auto &pair : filters) {
        if (pair.second) {
            anyFilterActive = true;
            break;
        }
    }
    m_occView->setMouseMode(anyFilterActive ? View::MouseMode::SELECTION : View::MouseMode::NONE);

    for (const auto &pair : filters) {
        m_occView->updateSelectionFilter(pair.first, pair.second);
    }
}

void ViewerWidget::updateSelectionFilter(TopAbs_ShapeEnum filter, bool isActive)
{
    m_occView->updateSelectionFilter(filter, isActive);

    // When a filter changes, we might need to update the mouse selection mode.
    // If any filter is active, selection mode should be on. Otherwise, it should be off.
    const auto &filters = m_occView->getSelectionFilters();
    bool anyFilterActive = false;
    for (const auto &pair : filters) {
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
    m_occView->addClippingPlane(point, normal);
}

void ViewerWidget::explosion()
{
    m_occView->checkInterference();
}

void ViewerWidget::measureDistance()
{
    const auto acitveView = ViewManager::getInstance().getActiveView();
    if (!acitveView)
        return;
    const auto &selectedList = acitveView->getSelectedObjects();
    if (selectedList.size() != 2) {
        QMessageBox::warning(
            this, QCoreApplication::translate("ViewerWidget", "Selection Error"),
            QCoreApplication::translate("ViewerWidget", "Please select exactly two vertices."));
        return;
    }

    TopoDS_Shape shape0 = selectedList.at(0)->GetSelectedShape();
    TopoDS_Shape shape1 = selectedList.at(1)->GetSelectedShape();
    if (shape0.IsNull() || shape1.IsNull() || shape0.ShapeType() != TopAbs_VERTEX
        || shape1.ShapeType() != TopAbs_VERTEX) {
        QMessageBox::warning(
            this, QCoreApplication::translate("ViewerWidget", "Selection Error"),
            QCoreApplication::translate("ViewerWidget", "Please select exactly two vertices."));
        return;
    }

    const TopoDS_Vertex &vertex0 = TopoDS::Vertex(shape0);
    const TopoDS_Vertex &vertex1 = TopoDS::Vertex(shape1);

    const auto point0 = BRep_Tool::Pnt(vertex0);
    const auto point1 = BRep_Tool::Pnt(vertex1);

    const Standard_Real distance = point0.Distance(point1);
    QMessageBox::information(
        this, QCoreApplication::translate("ViewerWidget", "Distance"),
        QCoreApplication::translate("ViewerWidget", "Distance between the two vertices: %1")
            .arg(distance));
}

void ViewerWidget::createPoint()
{
    DialogCreatePoint dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        gp_Pnt p(dlg.x(), dlg.y(), dlg.z());
        BRepBuilderAPI_MakeVertex vertexMaker(p);
        if (vertexMaker.IsDone()) {
            QColor c = dlg.color();
            displayShape(vertexMaker.Shape(), c.redF(), c.greenF(), c.blueF());
        }
    }
}

void ViewerWidget::createLine()
{
    DialogCreateLine dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        gp_Pnt p1(dlg.x1(), dlg.y1(), dlg.z1());
        gp_Pnt p2(dlg.x2(), dlg.y2(), dlg.z2());
        BRepBuilderAPI_MakeEdge edge(p1, p2);
        if (edge.IsDone()) {
            QColor c = dlg.color();
            displayShape(edge.Shape(), c.redF(), c.greenF(), c.blueF());
        }
    }
}

void ViewerWidget::createRectangle()
{
    DialogCreateRectangle dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        double x = dlg.x();
        double y = dlg.y();
        double z = dlg.z();
        double w = dlg.width();
        double h = dlg.height();

        gp_Pnt p1(x, y, z);
        gp_Pnt p2(x + w, y, z);
        gp_Pnt p3(x + w, y + h, z);
        gp_Pnt p4(x, y + h, z);

        BRepBuilderAPI_MakePolygon poly;
        poly.Add(p1);
        poly.Add(p2);
        poly.Add(p3);
        poly.Add(p4);
        poly.Add(p1); // Close the polygon
        if (poly.IsDone()) {
            BRepBuilderAPI_MakeFace face(poly.Wire());
            if (face.IsDone()) {
                QColor c = dlg.color();
                displayShape(face.Shape(), c.redF(), c.greenF(), c.blueF());
            }
        }
    }
}

void ViewerWidget::createCircle()
{
    DialogCreateCircle dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        gp_Ax2 axis(gp_Pnt(dlg.x(), dlg.y(), dlg.z()), gp_Dir(0, 0, 1)); // Z axis
        gp_Circ circle(axis, dlg.radius());
        BRepBuilderAPI_MakeEdge edge(circle);
        if (edge.IsDone()) {
            QColor c = dlg.color();
            displayShape(edge.Shape(), c.redF(), c.greenF(), c.blueF());
        }
    }
}

void ViewerWidget::createArc()
{
    gp_Pnt center(0, 0, 0);
    gp_Pnt p1(30, 0, 0);
    gp_Pnt p2(0, 30, 0);
    GC_MakeArcOfCircle arc(center, p1, p2);
    if (arc.IsDone()) {
        BRepBuilderAPI_MakeEdge edge(arc.Value());
        if (edge.IsDone()) {
            displayShape(edge.Shape(), 1.0, 1.0, 0.0);
        }
    }
}

void ViewerWidget::createEllipse()
{
    gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); // Z axis
    gp_Elips ellipse(axis, 30.0, 15.0);            // Major radius 30, minor 15
    BRepBuilderAPI_MakeEdge edge(ellipse);
    if (edge.IsDone()) {
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
    if (poly.IsDone()) {
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
    if (edge.IsDone()) {
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
    if (edge.IsDone()) {
        displayShape(edge.Shape(), 0.2, 0.8, 0.2);
    }
}

void ViewerWidget::createBox()
{
    DialogCreateBox dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        const gp_Pnt P1{dlg.x(), dlg.y(), dlg.z()};
        const gp_Pnt P2{dlg.x() + dlg.dx(), dlg.y() + dlg.dy(), dlg.z() + dlg.dz()};
        BRepPrimAPI_MakeBox box(P1, P2);
        QColor c = dlg.color();
        displayShape(box.Shape(), c.redF(), c.greenF(), c.blueF());
    }
}

void ViewerWidget::createPyramid()
{
}

void ViewerWidget::createSphere()
{
    BRepPrimAPI_MakeSphere sphere(gp_Pnt(0, 0, 0), 5.0);
    displayShape(sphere.Shape(), 1.0, 0.0, 0.0); // NOLINT
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
    if (!getBooleanTargets(shapeA, shapeB))
        return;

    BRepAlgoAPI_Fuse booleanFun(shapeA, shapeB);
    if (booleanFun.IsDone()) {
        TopoDS_Shape result = booleanFun.Shape();
        displayShape(result, 1.0, 0.0, 1.0);
        m_occView->clearSelectedObjects();
    } else {
        QMessageBox::warning(this, tr("Boolean Operation Failed"),
                             tr("Boolean Union operation failed!"));
    }
}

void ViewerWidget::booleanIntersection()
{
    TopoDS_Shape shape1;
    TopoDS_Shape shape2;
    if (!getBooleanTargets(shape1, shape2))
        return;
    BRepAlgoAPI_Common booleanFun(shape1, shape2);
    if (booleanFun.IsDone()) {
        TopoDS_Shape result = booleanFun.Shape();
        displayShape(result, 0.0, 1.0, 0.0);
        m_occView->clearSelectedObjects();
    } else {
        QMessageBox::warning(this, tr("Boolean Operation Failed"),
                             tr("Boolean Intersection operation failed!"));
    }
}

/*  https://github.com/bezierC0/CADHealingTool */
void ViewerWidget::repairAndSave(const TopoDS_Shape &shape)
{
    TopoDS_Shape repairedShape = shape;

    // 1.  ShapeFix_Shape
    qDebug() << "Step 1: Basic shape fixing...";
    Handle(ShapeFix_Shape) shapeFixer = new ShapeFix_Shape(repairedShape);
    shapeFixer->SetPrecision(1e-6);
    shapeFixer->SetMinTolerance(1e-7);
    shapeFixer->SetMaxTolerance(1.0);

    //
    shapeFixer->FixFreeShellMode() = 1;
    shapeFixer->FixFreeFaceMode() = 1;
    shapeFixer->FixFreeWireMode() = 1;
    shapeFixer->FixSameParameterMode() = 1;
    shapeFixer->FixVertexPositionMode() = 1;

    shapeFixer->Perform();
    repairedShape = shapeFixer->Shape();

    // 2. Fixing solids
    qDebug() << "Step 2: Fixing solids...";
    for (TopExp_Explorer exp(repairedShape, TopAbs_SOLID); exp.More(); exp.Next()) {
        TopoDS_Solid solid = TopoDS::Solid(exp.Current());
        Handle(ShapeFix_Solid) solidFixer = new ShapeFix_Solid();
        solidFixer->Init(solid);
        solidFixer->SetPrecision(1e-6);
        solidFixer->SetMaxTolerance(1.0);
        solidFixer->FixShellMode() = 1;
        solidFixer->Perform();
    }

    // 3. Fixing face
    qDebug() << "Step 3: Fixing face orientations...";
    for (TopExp_Explorer exp(repairedShape, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        Handle(ShapeFix_Face) faceFixer = new ShapeFix_Face(face);
        faceFixer->SetPrecision(1e-6);
        faceFixer->SetMaxTolerance(1.0);
        faceFixer->FixOrientationMode() = 1;
        faceFixer->FixAddNaturalBoundMode() = 1;
        faceFixer->FixMissingSeamMode() = 1;
        faceFixer->FixPeriodicDegeneratedMode() = 1;
        faceFixer->Perform();
    }

    // 4. Fixing wires
    qDebug() << "Step 4: Fixing wires...";
    for (TopExp_Explorer exp(repairedShape, TopAbs_WIRE); exp.More(); exp.Next()) {
        TopoDS_Wire wire = TopoDS::Wire(exp.Current());
        Handle(ShapeFix_Wire) wireFixer = new ShapeFix_Wire();
        wireFixer->Load(wire);
        wireFixer->SetPrecision(1e-6);
        wireFixer->SetMaxTolerance(1.0);
        wireFixer->FixReorder();
        wireFixer->FixConnected();
        wireFixer->FixEdgeCurves();
        wireFixer->FixDegenerated();
        wireFixer->FixSelfIntersection();
        wireFixer->FixLacking();
        wireFixer->FixClosed();
        wireFixer->FixGaps3d();
        wireFixer->FixGaps2d();
    }

    // 5. Removing small features
    qDebug() << "Step 5: Removing small features...";
    Handle(ShapeFix_FixSmallFace) smallFaceFixer = new ShapeFix_FixSmallFace();
    smallFaceFixer->Init(repairedShape);
    smallFaceFixer->SetPrecision(1e-6);
    smallFaceFixer->Perform();
    repairedShape = smallFaceFixer->Shape();

    // 6. Unifying same domain
    qDebug() << "Step 6: Unifying same domain...";
    ShapeUpgrade_UnifySameDomain unifier(repairedShape);
    unifier.SetLinearTolerance(1e-6);
    unifier.SetAngularTolerance(1e-4);
    unifier.AllowInternalEdges(Standard_False);
    unifier.SetSafeInputMode(Standard_True);
    unifier.Build();
    repairedShape = unifier.Shape();

    // 7.Simplifying shape
    qDebug() << "Step 7: Simplifying shape...";
    Handle(ShapeBuild_ReShape) reShape = new ShapeBuild_ReShape();
    reShape->Apply(repairedShape);
    repairedShape = reShape->Apply(repairedShape);

    // 8. Fixing tolerances.
    qDebug() << "Step 8: Fixing tolerances...";
    ShapeFix::SameParameter(repairedShape, Standard_False);

    // 9. Validating repaired shape.
    qDebug() << "Step 9: Validating repaired shape...";
    BRepCheck_Analyzer analyzer(repairedShape);
    if (!analyzer.IsValid()) {
        qDebug() << "Warning: Shape is still invalid after repair";

        qDebug() << "Attempting more aggressive repair...";
        BRepBuilderAPI_Sewing sewing(1e-6);
        sewing.Load(repairedShape);
        sewing.Perform();
        repairedShape = sewing.SewedShape();

        BRepCheck_Analyzer finalAnalyzer(repairedShape);
        if (finalAnalyzer.IsValid()) {
            qDebug() << "Shape repaired successfully after sewing";
        } else {
            qDebug() << "Shape remains invalid - saving anyway";
        }
    } else {
        qDebug() << "Shape is valid!";
    }

    // 10. Saving repaired shape
    qDebug() << "Step 10: Saving repaired shape...";
    STEPControl_Writer writer;
    writer.Transfer(repairedShape, STEPControl_AsIs);
    IFSelect_ReturnStatus status = writer.Write("fix.stp");

    if (status == IFSelect_RetDone) {
        QMessageBox::information(this, "Success",
                                 "Shape repaired and saved to fix.stp successfully!");
    } else {
        QMessageBox::warning(this, "Error", "Failed to write fix.stp");
    }
}

void ViewerWidget::booleanDifference()
{
    TopoDS_Shape shape1;
    TopoDS_Shape shape2;
    if (!getBooleanTargets(shape1, shape2))
        return;
    BRepAlgoAPI_Cut booleanFun(shape1, shape2);
    if (booleanFun.IsDone()) {
        TopoDS_Shape result = booleanFun.Shape();
        displayShape(result, 0.0, 0.0, 1.0);
        m_occView->clearSelectedObjects();
    } else {
        QMessageBox::warning(this, tr("Boolean Operation Failed"),
                             tr("Boolean Difference operation failed!"));
    }
}

void ViewerWidget::patternLinear()
{
    auto selectedObjects = m_occView->getSelectedObjects();
    if (selectedObjects.size() != 2) {
        QMessageBox::warning(
            this, QCoreApplication::translate("ViewerWidget", "Selection Error"),
            QCoreApplication::translate("ViewerWidget", "Please select exactly two shapes."));
        return;
    }

    const auto &shape0 = selectedObjects.at(0)->GetSelectedShape();
    const auto &shape1 = selectedObjects.at(1)->GetSelectedShape();

    if (shape0.IsNull() || shape1.IsNull()) {
        return;
    }

    if (shape0.ShapeType() != TopAbs_SHAPE && shape1.ShapeType() != TopAbs_EDGE) {
        return;
    }

    Standard_Real first, last;
    auto geomLine =
        Handle(Geom_Line)::DownCast(BRep_Tool::Curve(TopoDS::Edge(shape1), first, last));
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
    if (selectedObjects.size() != 2) {
        QMessageBox::warning(
            this, QCoreApplication::translate("ViewerWidget", "Selection Error"),
            QCoreApplication::translate("ViewerWidget", "Please select exactly two shapes."));
        return;
    }

    const auto &shape0 = selectedObjects.at(0)->GetSelectedShape();
    const auto &shape1 = selectedObjects.at(1)->GetSelectedShape();

    if (shape0.IsNull() || shape1.IsNull()) {
        return;
    }

    if (shape0.ShapeType() != TopAbs_SHAPE && shape1.ShapeType() != TopAbs_EDGE) {
        return;
    }

    Standard_Real first, last;
    auto geomCircle =
        Handle(Geom_Circle)::DownCast(BRep_Tool::Curve(TopoDS::Edge(shape1), first, last));
    if (geomCircle.IsNull()) {
        return;
    }

    const TopoDS_Shape baseShape = shape0;
    const gp_Ax1 axis = geomCircle->Axis();
    Standard_Real angleStep{10.0};
    Standard_Integer count{3};
    m_occView->patternCircular(baseShape, axis, angleStep, count);
}

void ViewerWidget::mirrorByPlane()
{
    auto selectedObjects = m_occView->getSelectedObjects();
    if (selectedObjects.size() != 2) {
        QMessageBox::warning(
            this, QCoreApplication::translate("ViewerWidget", "Selection Error"),
            QCoreApplication::translate("ViewerWidget", "Please select exactly two shapes."));
        return;
    }

    const auto shape = selectedObjects.at(0)->GetSelectedShape();
    const auto plane = selectedObjects.at(1)->GetSelectedShape();

    if (shape.IsNull() || plane.IsNull()) {
        return;
    }

    if (shape.ShapeType() != TopAbs_SHAPE && plane.ShapeType() != TopAbs_FACE) {
        return;
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
    if (selectedObjects.size() != 2) {
        QMessageBox::warning(
            this, QCoreApplication::translate("ViewerWidget", "Selection Error"),
            QCoreApplication::translate("ViewerWidget", "Please select exactly two shapes."));
        return;
    }
    const auto shape0 = selectedObjects.at(0)->GetSelectedShape();
    const auto shape1 = selectedObjects.at(1)->GetSelectedShape();

    if (shape0.IsNull() || shape1.IsNull()) {
        return;
    }

    if (shape0.ShapeType() != TopAbs_SHAPE && shape1.ShapeType() != TopAbs_EDGE) {
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

void ViewerWidget::shell()
{
    const auto acitveView = ViewManager::getInstance().getActiveView();
    if (!acitveView)
        return;
    const auto &selectedList = acitveView->getSelectedObjects();
    if (selectedList.size() != 1) {
        QMessageBox::warning(
            this, QCoreApplication::translate("ViewerWidget", "Selection Error"),
            QCoreApplication::translate("ViewerWidget", "Please select exactly one face."));
        return;
    }

    const auto shape0 = selectedList.at(0)->GetSelectedShape();
    if (shape0.ShapeType() != TopAbs_FACE) {
        QMessageBox::warning(this, QCoreApplication::translate("ViewerWidget", "Selection Error"),
                             QCoreApplication::translate("ViewerWidget", "Type Error."));
        return;
    }
    const auto boxObject = selectedList.at(0)->GetParentInteractiveObject();
    const auto aisBox = Handle(AIS_Shape)::DownCast(boxObject);
    const auto box = aisBox->Shape();
    if (box.ShapeType() != TopAbs_SOLID) {
        QMessageBox::warning(this, QCoreApplication::translate("ViewerWidget", "Selection Error"),
                             QCoreApplication::translate("ViewerWidget", "Type Error."));
        return;
    }

    Quantity_Color color;
    boxObject->Color(color);

    TopoDS_Face face = TopoDS::Face(shape0);
    Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
    Handle(Geom_Plane) geomPlane = Handle(Geom_Plane)::DownCast(surface);
    if (geomPlane.IsNull()) {
        return;
    }
    TopTools_ListOfShape facesToRemove;
    Standard_Real maxZ = -1e10;
    TopoDS_Face topFace;
    for (TopExp_Explorer exp(box, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());

        GProp_GProps props;
        BRepGProp::SurfaceProperties(face, props);
        gp_Pnt center = props.CentreOfMass();

        if (center.Z() > maxZ) {
            maxZ = center.Z();
            topFace = face;
        }
    }
    facesToRemove.Append(topFace);
    m_occView->clearSelectedObjects();
    auto newShape = m_occView->shell(box, facesToRemove);
    removeShape(box);
    displayShape(newShape, color.Red(), color.Green(), color.Blue());
}

void ViewerWidget::displayShape(const TopoDS_Shape &shape, const double r, const double g,
                                const double b)
{
    // Add to OCAF Document
    TDF_Label label;
    if (!m_doc->m_ocafDoc.IsNull()) {
        Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(m_doc->m_ocafDoc->Main());
        Handle(XCAFDoc_ColorTool) colorTool = XCAFDoc_DocumentTool::ColorTool(m_doc->m_ocafDoc->Main());
        
        label = shapeTool->AddShape(shape, false); // Add as simple shape
        
        // Set a name
        std::string typeName = getShapeTypeString(shape);
        TDataStd_Name::Set(label, typeName.c_str());

        // Set Color
        Quantity_Color color(r, g, b, Quantity_TOC_RGB);
        colorTool->SetColor(label, color, XCAFDoc_ColorGen);
        colorTool->SetColor(label, color, XCAFDoc_ColorSurf);
        colorTool->SetColor(label, color, XCAFDoc_ColorCurv);
        
        // Update the Tree View
        updateTree();
    }

    // Display
    // Use XCAFPrs_AISObject if we have a label, otherwise fallback to AIS_Shape (though we should have a label now)
    if (!label.IsNull()) {
        Handle(XCAFPrs_AISObject) aisObj = new XCAFPrs_AISObject(label);
        aisObj->SetDisplayMode(AIS_Shaded);
        // aisObj->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB)); // Let OCAF handle color
        m_occView->setShape(aisObj);
        
        // Add to m_doc list to track it
        m_doc->m_list.emplace_back(aisObj);

    } else {
        Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
        aisShape->SetDisplayMode(AIS_Shaded);
        aisShape->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
        m_occView->setShape(aisShape);
        m_doc->m_list.emplace_back(aisShape);
    }
    
    m_occView->reDraw();
    m_occView->viewfit(); // Fit view to the new shape
}

void ViewerWidget::removeShape(const TopoDS_Shape &shape)
{
    m_occView->removeShape(shape);
    m_occView->reDraw();
    m_occView->viewfit(); // Fit view to the new shape
}

const std::map<TopAbs_ShapeEnum, bool> &ViewerWidget::getSelectionFilters() const
{
    const auto acitveView = ViewManager::getInstance().getActiveView();
    assert(acitveView);
    return acitveView->getSelectionFilters();
}

bool ViewerWidget::getBooleanTargets(TopoDS_Shape &target1, TopoDS_Shape &target2)
{
    const auto acitveView = ViewManager::getInstance().getActiveView();
    if (!acitveView)
        return false;
    const auto &selectedList = acitveView->getSelectedAisShape(2);
    if (selectedList.size() != 2)
        return false;

    const auto aisShapeA = selectedList.at(0);
    const auto aisshapeB = selectedList.at(1);
    if (aisShapeA.IsNull() || aisshapeB.IsNull())
        return false;

    const auto shapeA = aisShapeA->Shape();
    const auto shapeB = aisshapeB->Shape();
    if (shapeA.IsNull() || shapeB.IsNull())
        return false;
    target1 = shapeA;
    target2 = shapeB;
    return true;
}

// TODO : BUG assembly highLight
void ViewerWidget::highlightLabel(const TDF_Label& label)
{
    if (label.IsNull()) return;

    // Retrieve the shape from the label
    TopoDS_Shape shape;
    if (XCAFDoc_ShapeTool::GetShape(label, shape)) {
        // Clear previous selection
        m_occView->clearSelectedObjects();
        // Remove previous temporary highlight object if any
        if (!m_highlightedShape.IsNull()) {
            m_occView->Context()->Remove(m_highlightedShape, false);
            m_highlightedShape.Nullify();
        }

        Handle(AIS_InteractiveContext) context = m_occView->Context();
        bool foundAsRoot = false;
        
        // Root / Top-level object 
        for (const auto& obj : m_doc->m_list) {
            Handle(XCAFPrs_AISObject) xcafObj = Handle(XCAFPrs_AISObject)::DownCast(obj);
            if (!xcafObj.IsNull()) {
                if (xcafObj->GetLabel().IsEqual(label)) {
                     if (!context.IsNull()) {
                        context->SetSelected(obj, true);
                    }
                    foundAsRoot = true;
                    break;
                }
            }
        }
        
        // not root object 
        if (!foundAsRoot) {
            Handle(AIS_Shape) tempObj = new AIS_Shape(shape);
            tempObj->SetColor(Quantity_NOC_GOLD); 
            tempObj->SetWidth(2.0);
            
            // Display it
            context->Display(tempObj, false); 
            // Select it (this applies the selection highlight style)
            context->SetSelected(tempObj, true);
            
            // Store it so we can remove it later
            m_highlightedShape = tempObj;
        }
        
        m_occView->reDraw(); // Keep this for standard OCCT redraw
        m_occView->update(); // Force Qt widget update immediately
    }
}

