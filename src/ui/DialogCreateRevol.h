#pragma once

#include <QColor>
#include <QDialog>
#include <TopoDS_Shape.hxx>

class QCloseEvent;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class SelectionPickSession;

class DialogCreateRevol : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateRevol(QWidget* parent = nullptr);
    ~DialogCreateRevol() override;
    void show();

signals:
    void signalCreateRevol(const TopoDS_Shape& face, double x, double y, double z,
                           double nx, double ny, double nz, double angle, const QColor& color);

protected:
    void done(int result) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onPickClicked();
    void onShapePicked(const TopoDS_Shape& shape);
    void onCreateClicked();
    void onColorClicked();

private:
    SelectionPickSession* m_pickSession{nullptr};
    TopoDS_Shape m_face;
    QLabel* m_status{nullptr};
    QDoubleSpinBox* m_x{nullptr};
    QDoubleSpinBox* m_y{nullptr};
    QDoubleSpinBox* m_z{nullptr};
    QDoubleSpinBox* m_nx{nullptr};
    QDoubleSpinBox* m_ny{nullptr};
    QDoubleSpinBox* m_nz{nullptr};
    QDoubleSpinBox* m_angle{nullptr};
    QColor m_color{Qt::white};
    QPushButton* m_colorButton{nullptr};
};
