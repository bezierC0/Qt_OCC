#pragma once

#include <map>
#include <vector>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>

#include <QWidget>

class QCloseEvent;
class TopoDS_Edge;

namespace Ui
{
class WidgetMeasureArcLength;
}

class WidgetMeasureArcLength : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetMeasureArcLength(QWidget *parent = nullptr);
    ~WidgetMeasureArcLength() override;

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
    void saveMouseState();
    void restoreMouseState();
    void updateUI();
    bool isArcEdge(const TopoDS_Shape &shape) const;
    void calculateAndAddArcLength(const TopoDS_Shape &shape);

private:
    Ui::WidgetMeasureArcLength *ui;
    int m_savedMouseMode;
    std::map<TopAbs_ShapeEnum, bool> m_savedFilters;
    bool m_isPicking;
    std::vector<TopoDS_Shape> m_selectedShapes;
    double m_totalArcLength;
};
