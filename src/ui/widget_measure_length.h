#pragma once

#include <map>
#include <vector>

#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>

#include <QWidget>

class QCloseEvent;

namespace Ui
{
class WidgetMeasureLength;
}

class WidgetMeasureLength : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetMeasureLength(QWidget *parent = nullptr);
    ~WidgetMeasureLength() override;

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
    void calculateAndAddLength(const TopoDS_Shape &shape);

private:
    Ui::WidgetMeasureLength *ui;

    // State saving
    int m_savedMouseMode;
    std::map<TopAbs_ShapeEnum, bool> m_savedFilters;

    bool m_isPicking;
    std::vector<TopoDS_Shape> m_selectedShapes;
    double m_totalLength;
};
