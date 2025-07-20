#pragma once

#include <QWidget>
#include <TopoDS_Shape.hxx> // Add this include

#include "OCCView.h" // Add this for InteractionMode enum
class AIS_InteractiveObject;
class gp_Dir;
class gp_Pnt;

template<typename T>
class Tree;
namespace opencascade
{
    template <class T>
    class handle;
}

class ViewerWidget : public QWidget
{
    Q_OBJECT
    struct Document{
        std::vector<opencascade::handle<AIS_InteractiveObject>> m_list;
    };

public:
    explicit ViewerWidget(QWidget* parent = nullptr);
    ~ViewerWidget() override;

    void loadModel(const QString& filename) const;

    //view
    void viewFit();
    void viewIsometric() const;
    void viewTop() const ;
    void viewBottom() const ;
    void viewLeft() const ;
    void viewRight() const ;
    void viewFront() const ;
    void viewBack() const ;
    void setDisplayMode( int mode ) ;

    void switchSelect(bool checked);
    void setFilters(const std::map<TopAbs_ShapeEnum, bool>& filters);
    void updateSelectionFilter(TopAbs_ShapeEnum filter, bool isActive);

    // tool
    void checkInterference();
    void transform();
    void clipping(const gp_Dir& normal, const gp_Pnt& point, bool isOn = true);
    void explosion();
    void measureDistance();

    /* shape */
    void createPoint();
    void createLine();
    void createRectangle();
    void createCircle();
    void createArc();
    void createEllipse();
    void createPolygon();
    void createBezierCurve();
    void createNurbsCurve();
    void createBox();
    void createPyramid();
    void createSphere();
    void createCylinder();
    void createCone();
    void booleanUnion();
    void booleanIntersection();
    void booleanDifference();
    void patternLinear();// Pattern(linear) 
    void patternCircular();// Pattern(Circular) 
    void mirrorByPlane();
    void mirrorByAxis();
    
    void displayShape(const TopoDS_Shape& shape, double r = 1.0, double g=1.0, double b=1.0); // Add this function
    void removeShape(const TopoDS_Shape& shape);
    const std::map<TopAbs_ShapeEnum, bool>& getSelectionFilters() const ;

private:
    bool getBooleanTargets(TopoDS_Shape& target1, TopoDS_Shape& target2);

private:
    OCCView* m_occView;
    std::shared_ptr<Document> m_doc;

    //TopoDS_Shape m_loadedShape;
};
