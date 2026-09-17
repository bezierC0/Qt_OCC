#pragma once

#include <QColor>
#include <QDialog>
#include <QPointer>

#include <AIS_Shape.hxx>
#include <TopoDS_Shape.hxx>

class OCCView;
class SelectionPickSession;
class QDoubleSpinBox;
class QLabel;
class QPushButton;

class DialogCreateOffsetCurve : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateOffsetCurve(QWidget* parent = nullptr);
    ~DialogCreateOffsetCurve() override;
    void show();
    void done(int result) override;

signals:
    void signalCreateOffsetCurve(const TopoDS_Shape& basis, double distance,
                                 double nx, double ny, double nz, const QColor& color);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void onPickClicked();
    void onShapePicked(const TopoDS_Shape& shape);
    void updatePreview();
    void clearPreview();
    void onCreateClicked();
    void onColorClicked();

    TopoDS_Shape m_basis;
    QPointer<OCCView> m_previewView;
    Handle(AIS_Shape) m_previewShape;
    SelectionPickSession* m_pickSession{nullptr};
    QDoubleSpinBox* m_distance{nullptr};
    QDoubleSpinBox* m_normalX{nullptr};
    QDoubleSpinBox* m_normalY{nullptr};
    QDoubleSpinBox* m_normalZ{nullptr};
    QLabel* m_status{nullptr};
    QPushButton* m_colorButton{nullptr};
    QColor m_color{Qt::white};
};
