#pragma once

#include <QWidget>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <TopoDS_Shape.hxx>

class OCCView;

class ViewerWidget : public QWidget 
{
    Q_OBJECT
    struct Document{
        std::vector<TopoDS_Shape> m_list;
    };

public:
    explicit ViewerWidget(QWidget* parent = nullptr);
    ~ViewerWidget() override;
    void loadModel(const QString& filename) const;
    void setTopView();
    void checkInterference();
    void clipping();
    void explosion();

private:
    OCCView* m_occView;
    std::shared_ptr<Document> m_doc;

    //TopoDS_Shape m_loadedShape;
};
