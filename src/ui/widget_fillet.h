#pragma once
#include <QWidget>
#include <TopoDS_Shape.hxx>

class QDoubleSpinBox;
class QPushButton;
class QLabel;
class SelectionPickSession;

class WidgetFillet : public QWidget {
    Q_OBJECT
public:
    explicit WidgetFillet(QWidget* parent = nullptr);
    ~WidgetFillet() override;

    void show();
    void hide();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onPickClicked();
    void onObjectSelected(const TopoDS_Shape& shape);
    void onApplyClicked();

signals:
    void signalFillet(const TopoDS_Shape& edge, double radius);

private:
    void restoreMouseState();

    SelectionPickSession* m_pickSession;
    TopoDS_Shape m_selectedEdge;

    QPushButton* btnPick;
    QPushButton* btnApply;
    QLabel* lblStatus;
    QDoubleSpinBox* spinRadius;
};
