#pragma once

#include <QWidget>
#include <QCloseEvent>
#include <TopAbs_ShapeEnum.hxx>
#include <map>
#include <TopoDS_Shape.hxx>

class QLabel;
class QCloseEvent;

namespace Ui {
class WidgetSetCoordinateSystem;
}

class WidgetSetCoordinateSystem : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetSetCoordinateSystem(QWidget *parent = nullptr);
    ~WidgetSetCoordinateSystem();
    void closeEvent(QCloseEvent *event) override;

public Q_SLOTS:
    void show();
    void hide();

private slots:
    void onCoordinateChanged();
    void onPushButtonCancel();
    void onPickPointClicked();
    void onPickNormalClicked();
    void onObjectSelected(const TopoDS_Shape& shape);

private:
    void createWorkPlane();
    void saveMouseState();
    void restoreMouseState();

private:
    enum class PickMode                 { None, Point, Face };
    Ui::WidgetSetCoordinateSystem*      ui;
    QLabel*                             m_labelResultNormal;
    class CoordinateSystemShape*        m_previewCoordSys;
    PickMode                            m_pickMode = PickMode::None;
    
    // State saving
    int                                 m_savedMouseMode;
    std::map<TopAbs_ShapeEnum, bool>    m_savedFilters;
};
