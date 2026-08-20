#pragma once

#include <vector>
#include <TopoDS_Shape.hxx>

#include <QWidget>

class QCloseEvent;
class SelectionPickSession;

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
    void restoreMouseState();
    void updateUI();
    void calculateAndAddLength(const TopoDS_Shape &shape);

private:
    Ui::WidgetMeasureLength *ui;

    SelectionPickSession* m_pickSession;
    std::vector<TopoDS_Shape> m_selectedShapes;
    double m_totalLength;
};
