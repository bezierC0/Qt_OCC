#pragma once

#include <QDialog>
#include <QColor>

class QDoubleSpinBox;
class QPushButton;

class DialogCreateSphere : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateSphere(QWidget* parent = nullptr);
    ~DialogCreateSphere() override;

    double x() const;
    double y() const;
    double z() const;
    double radius() const;
    QColor color() const;

signals:
    void signalCreateSphere(double x, double y, double z, double radius, const QColor& color);

private slots:
    void onBtnOkClicked();
    void onBtnColorClicked();

private:
    QDoubleSpinBox*     m_spinBoxX;
    QDoubleSpinBox*     m_spinBoxY;
    QDoubleSpinBox*     m_spinBoxZ;
    
    QDoubleSpinBox*     m_spinBoxRadius;

    QColor              m_color;
    QPushButton*        m_btnColor;
};
