#pragma once

#include <QWidget>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>
#include "Tree.h"


class OCCView;

class ViewerWidget : public QWidget 
{
    Q_OBJECT
    struct Document{
        std::vector<Handle(AIS_InteractiveObject)> m_list;
    };

public:
    explicit ViewerWidget(QWidget* parent = nullptr);
    ~ViewerWidget() override;
    void loadModel(const QString& filename) const;
    void viewFit();
    void checkInterference();
    void clipping( const gp_Dir& normal, const gp_Pnt& point, bool isOn = true );
    void explosion();

private:
    OCCView* m_occView;
    std::shared_ptr<Document> m_doc;
    

    //TopoDS_Shape m_loadedShape;
};
