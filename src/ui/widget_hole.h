#pragma once

#include <map>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>

#include <QWidget>

class QCloseEvent;

namespace Ui
{
class WidgetHole;
}

class WidgetHole : public QWidget
{
    Q_OBJECT

public:
    enum class HoleType
    {
        ThruAll,   // Perform(radius) 
        ThruNext,  // PerformThruNext(radius) 
        UntilEnd,  // PerformUntilEnd(radius) 
        Blind      // PerformBlind(radius, depth) 
    };

    explicit WidgetHole(QWidget* parent = nullptr);
    ~WidgetHole() override;

    void show();
    void hide();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onPickFaceClicked();
    void onPickPointClicked();
    void onObjectSelected(const TopoDS_Shape& shape);
    void onApplyClicked();
    void onCloseClicked();

signals:
    void signalHole(const TopoDS_Shape& parentShape,
                    const TopoDS_Shape& face,
                    const TopoDS_Shape& point,
                    double radius,
                    int holeType,
                    double depth);

private:
    void saveMouseState();
    void restoreMouseState();

    Ui::WidgetHole* ui;

    int m_savedMouseMode;
    std::map<TopAbs_ShapeEnum, bool> m_savedFilters;

    enum PickingState
    {
        Idle,
        PickFace,
        PickPoint
    };
    PickingState m_pickingState;

    TopoDS_Shape m_parentShape;
    TopoDS_Shape m_selectedFace;
    TopoDS_Shape m_selectedPoint;
};
