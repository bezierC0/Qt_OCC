#pragma once
#include <Standard_Handle.hxx>

class AIS_Shape;
class TopoDS_Shape;
class Quantity_Color;
class AIS_InteractiveContext;
class CollisionDetector {
private:
    Handle(AIS_InteractiveContext) myAISContext;
    
public:
    CollisionDetector(Handle(AIS_InteractiveContext) theContext) ;
    
    bool DetectAndHighlightCollision(const TopoDS_Shape& shape1, 
                                   const TopoDS_Shape& shape2);

    void AdvancedCollisionVisualization(const TopoDS_Shape& shape1, 
                                      const TopoDS_Shape& shape2) ;
    
    Handle(AIS_Shape) GetResult() const;

private:
    bool IsShapeEmpty(const TopoDS_Shape& shape) ;

    const TopoDS_Shape& MeshShape(const TopoDS_Shape& shape, double deflection = 0.1) ;

    void SetCollisionAppearance(Handle(AIS_Shape) aisShape) ;

private:
    Handle(AIS_Shape) m_result { nullptr };
};