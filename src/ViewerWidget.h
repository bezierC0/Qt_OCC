#pragma once

#include <QWidget>

class OCCView;
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
    void viewFit();
    void checkInterference();
    void transform();
    void clipping( const gp_Dir& normal, const gp_Pnt& point, bool isOn = true );
    void explosion();

private:
    OCCView* m_occView;
    std::shared_ptr<Document> m_doc;

    //TopoDS_Shape m_loadedShape;
};
