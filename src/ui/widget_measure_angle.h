#pragma once

#include <map>
#include <vector>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>

#include <QWidget>

class QCloseEvent;

namespace Ui
{
class WidgetMeasureAngle;
}

class WidgetMeasureAngle : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetMeasureAngle(QWidget *parent = nullptr);
    ~WidgetMeasureAngle() override;

    void show();
    void hide();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onPickClicked();
    void onClearClicked();
    void onCloseClicked();
    void onObjectSelected(const TopoDS_Shape &shape);

private:
    enum class PickMode
    {
        Unknown,
        EdgeAngle,
        VertexAngle
    };

    void saveMouseState();
    void restoreMouseState();
    void resetMeasurement();
    void updateUI();
    bool isLineEdge(const TopoDS_Shape &shape) const;
    bool isVertexShape(const TopoDS_Shape &shape) const;
    bool canAcceptShape(const TopoDS_Shape &shape) const;
    int requiredSelectionCount() const;
    bool tryLoadFromCurrentSelection();
    bool calculateAngle();

private:
    Ui::WidgetMeasureAngle *ui;
    int m_savedMouseMode;
    std::map<TopAbs_ShapeEnum, bool> m_savedFilters;
    bool m_isPicking;
    PickMode m_pickMode;
    std::vector<TopoDS_Shape> m_selectedShapes;
    double m_angleDeg;
    double m_angleRad;
    bool m_hasMeasurement;
};
