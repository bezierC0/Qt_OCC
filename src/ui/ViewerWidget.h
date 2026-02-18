#pragma once

#include <string>
#include <QWidget>
#include <TopoDS_Shape.hxx> 
#include <TDocStd_Document.hxx>
#include "OCCView.h" 
class AIS_InteractiveObject;
class gp_Dir;
class gp_Pnt;

class DialogCreateArc;
class DialogCreateBox;
class DialogCreateCircle;
class DialogCreateCone;
class DialogCreateCylinder;
class DialogCreateEllipse;
class DialogCreateLine;
class DialogCreatePoint;
class DialogCreateRectangle;
class DialogCreateSphere;
class DialogCreatePolygon;
class DialogCreateBezier;
class DialogCreateNurbs;
class DialogExportImage;
class WidgetInterference;
class WidgetDistance;

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
    void exportModel(const QString& filename);

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

    void exportPicture();
    void onFunctionTest();
public slots:
    void highlightLabel(const TDF_Label& label);

private slots:
    void onCreateArc(double x1, double y1, double z1, double x2, double y2, double z2, double x3, double y3, double z3, const QColor& color);
    void onCreateBox(double x, double y, double z, double dx, double dy, double dz, const QColor& color);
    void onCreateCircle(double x, double y, double z, double radius, const QColor& color);
    void onCreateCone(double x, double y, double z, double r1, double r2, double h, const QColor& color);
    void onCreateCylinder(double x, double y, double z, double r, double h, const QColor& color);
    void onCreateEllipse(double cx, double cy, double cz, double nx, double ny, double nz, double major, double minor, const QColor& color);
    void onCreateLine(double x1, double y1, double z1, double x2, double y2, double z2, const QColor& color);
    void onCreatePoint(double x, double y, double z, const QColor& color);
    void onCreateRectangle(double x, double y, double z, double width, double height, const QColor& color);
    void onCreateSphere(double x, double y, double z, double radius, const QColor& color);
    void onCreatePolygon(const QList<gp_Pnt>& points, bool isClosed, const QColor& color);
    void onCreateBezier(const QList<gp_Pnt>& points, const QColor& color);
    void onCreateNurbs(const QList<gp_Pnt>& points, int degree, const QColor& color);
    
private:
    bool getBooleanTargets(TopoDS_Shape& target1, TopoDS_Shape& target2);

private:
    OCCView*                        m_occView{nullptr};
    std::shared_ptr<Document>       m_doc{nullptr};
    bool                            m_importWithHealing{false};
    Handle(AIS_InteractiveObject)   m_highlightedShape{nullptr};
    bool                            m_isShowBoundingBox{true};  

    //TopoDS_Shape m_loadedShape;

    DialogCreateArc*      m_dlgArc{nullptr};
    DialogCreateBox*      m_dlgBox{nullptr};
    DialogCreateCircle*   m_dlgCircle{nullptr};
    DialogCreateCone*     m_dlgCone{nullptr};
    DialogCreateCylinder* m_dlgCylinder{nullptr};
    DialogCreateEllipse*  m_dlgEllipse{nullptr};
    DialogCreateLine*     m_dlgLine{nullptr};
    DialogCreatePoint*    m_dlgPoint{nullptr};
    DialogCreateRectangle* m_dlgRectangle{nullptr};
    DialogCreateSphere*   m_dlgSphere{nullptr};
    DialogCreatePolygon*  m_dlgPolygon{nullptr};
    DialogCreateBezier*   m_dlgBezier{nullptr};
    DialogCreateNurbs*    m_dlgNurbs{nullptr};
    DialogExportImage*    m_dlgExportImage{nullptr};
    WidgetInterference*   m_widgetInterference{nullptr};
    WidgetDistance*       m_widgetDistance{nullptr};
};

