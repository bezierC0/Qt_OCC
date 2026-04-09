#pragma once
#include <QWidget>
#include <TopoDS_Shape.hxx>
#include <map>
#include <TopAbs_ShapeEnum.hxx>

class QDoubleSpinBox;
class QPushButton;
class QLabel;

class WidgetChamfer : public QWidget {
    Q_OBJECT
public:
    explicit WidgetChamfer(QWidget* parent = nullptr);
    ~WidgetChamfer() override;

    void show();
    void hide();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onPickClicked();
    void onObjectSelected(const TopoDS_Shape& shape);
    void onApplyClicked();

signals:
    void signalChamfer(const TopoDS_Shape& edge, double distance);

private:
    void saveMouseState();
    void restoreMouseState();

    int m_savedMouseMode;
    std::map<TopAbs_ShapeEnum, bool> m_savedFilters;
    bool m_isPicking;
    TopoDS_Shape m_selectedEdge;

    QPushButton* btnPick;
    QPushButton* btnApply;
    QLabel* lblStatus;
    QDoubleSpinBox* spinDistance;
};
