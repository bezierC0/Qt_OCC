#include "CollisionDetector.h"
#include <cassert>

#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_PolygonOffsetMode.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <Quantity_Color.hxx>
#include <V3d_View.hxx>

CollisionDetector::CollisionDetector(Handle(AIS_InteractiveContext) theContext) : myAISContext(theContext)
{
}


bool CollisionDetector::DetectAndHighlightCollision(const TopoDS_Shape &shape1, const TopoDS_Shape &shape2)
{
    BRepAlgoAPI_Common commonOp(shape1, shape2);
    commonOp.Build();

    if (!commonOp.IsDone()) {
        return false;
    }

    TopoDS_Shape intersectionShape = commonOp.Shape();

    if (intersectionShape.IsNull() || IsShapeEmpty(intersectionShape)) {
        return false;
    }

    const auto& resultMesh = MeshShape(intersectionShape);

    //m_result = new AIS_Shape(resultMesh);

    m_result = new AIS_Shape(intersectionShape);

    // SetCollisionAppearance(aisCollision);

    //AdvancedCollisionVisualization(shape1, shape2);

    //myAISContext->Display(aisCollision, Standard_True);

    return true;
}


void CollisionDetector::AdvancedCollisionVisualization(const TopoDS_Shape &shape1, const TopoDS_Shape &shape2)
{
    constexpr double transparency = 0.7;

    Handle(AIS_Shape) aisShape1 = new AIS_Shape(shape1);
    Handle(AIS_Shape) aisShape2 = new AIS_Shape(shape2);

    aisShape1->SetTransparency(transparency);
    aisShape2->SetTransparency(transparency);

    myAISContext->Display(aisShape1, Standard_False);
    myAISContext->Display(aisShape2, Standard_False);

    //DetectAndHighlightCollision(shape1, shape2);

    myAISContext->UpdateCurrentViewer();
}

Handle(AIS_Shape) CollisionDetector::GetResult() const{
    return m_result;
}

bool CollisionDetector::IsShapeEmpty(const TopoDS_Shape &shape)
{
    TopExp_Explorer exp(shape, TopAbs_VERTEX);
    return !exp.More();
}

const TopoDS_Shape& CollisionDetector::MeshShape(const TopoDS_Shape &shape, double deflection)
{
    BRepMesh_IncrementalMesh mesh(shape, deflection);
    if (!mesh.IsDone()) {
        assert(false);
    }
    return mesh.Shape();
}


void CollisionDetector::SetCollisionAppearance(Handle(AIS_Shape) aisShape)
{
    //aisShape->SetColor(color);

    Graphic3d_MaterialAspect material(Graphic3d_NOM_PLASTIC);
    // material.SetAmbientColor(color);
    // material.SetDiffuseColor(color);
    // material.SetEmissiveColor(color * 0.3);
    material.SetShininess(0.8);

    aisShape->SetMaterial(material);

    aisShape->SetDisplayMode(AIS_Shaded);

    //aisShape->Attributes()->SetPolygonOffsets(Aspect_POM_Fill, 1.0, 0.005);
}