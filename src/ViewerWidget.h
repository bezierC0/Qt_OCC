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
    void setFilters(bool isElementOn);

    // tool
    void checkInterference();
    void transform();
    void clipping(const gp_Dir& normal, const gp_Pnt& point, bool isOn = true);
    void explosion();
    void measureDistance();
    
    void displayShape(const TopoDS_Shape& shape, double r = 1.0, double g=1.0, double b=1.0); // Add this function

private:
    OCCView* m_occView;
    std::shared_ptr<Document> m_doc;

    //TopoDS_Shape m_loadedShape;
};
