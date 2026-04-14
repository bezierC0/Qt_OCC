#pragma once
#include <QWidget>
#include <TopoDS_Shape.hxx>
#include <map>
#include <TopAbs_ShapeEnum.hxx>

class QDoubleSpinBox;
class QPushButton;
class QLabel;

class WidgetHole : public QWidget {
    Q_OBJECT
public:
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

signals:
    void signalHole(const TopoDS_Shape& parentShape, const TopoDS_Shape& face, const TopoDS_Shape& point, double radius);

private:
    void saveMouseState();
    void restoreMouseState();

    int m_savedMouseMode;
    std::map<TopAbs_ShapeEnum, bool> m_savedFilters;
    
    enum PickingState {
        Idle,
        PickFace,
        PickPoint
    };
    PickingState m_pickingState;
    
    TopoDS_Shape m_parentShape;
    TopoDS_Shape m_selectedFace;
    TopoDS_Shape m_selectedPoint;

    QPushButton* btnPickFace;
    QPushButton* btnPickPoint;
    QPushButton* btnApply;
    QLabel* lblStatus;
    QDoubleSpinBox* spinRadius;
};
