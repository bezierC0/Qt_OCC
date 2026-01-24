#pragma once

#include <string>
#include <QWidget>
#include <TopoDS_Shape.hxx> 
#include <TDocStd_Document.hxx>
#include "OCCView.h" 
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
    /* mayo Document
    struct Document{
        std::vector<opencascade::handle<AIS_InteractiveObject>> m_list;
    };
    */

    struct Document;


public:
    explicit ViewerWidget(QWidget* parent = nullptr);
    ~ViewerWidget() override;

    void clearAll();
    void loadModel(const QString& filename);

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

    // tool
    void switchSelect(bool checked);
    void setFilters(const std::map<TopAbs_ShapeEnum, bool>& filters);
    void updateSelectionFilter(TopAbs_ShapeEnum filter, bool isActive);
    void createWorkPlane();
    void checkInterference();
    void transform();
    void clipping(const gp_Dir& normal, const gp_Pnt& point, bool isOn = true);
    void explosion();
    
    /* measure */
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
    void shell();

    void displayShape(const TopoDS_Shape& shape, double r = 1.0, double g=1.0, double b=1.0); // Add this function
    void removeShape(const TopoDS_Shape& shape);
    const std::map<TopAbs_ShapeEnum, bool>& getSelectionFilters() const ;
    void repairAndSave(const TopoDS_Shape& shape);
    void updateTree();
public slots:
    void highlightLabel(const TDF_Label& label);

private:
    bool getBooleanTargets(TopoDS_Shape& target1, TopoDS_Shape& target2);

private:
    OCCView*                        m_occView{nullptr};
    std::shared_ptr<Document>       m_doc{nullptr};
    bool                            m_importWithHealing{false};
    Handle(AIS_InteractiveObject)   m_highlightedShape{nullptr};

    //TopoDS_Shape m_loadedShape;
};


