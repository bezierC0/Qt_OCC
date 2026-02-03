#pragma once

#include <QDialog>
#include <QColor>

class QDoubleSpinBox;
class QPushButton;

class DialogCreateCylinder : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateCylinder(QWidget* parent = nullptr);
    ~DialogCreateCylinder() override;

    double x() const;
    double y() const;
    double z() const;
    double radius() const;
    double height() const;
    QColor color() const;

signals:
    void signalCreateCylinder(double x, double y, double z, double r, double h, const QColor& color);

private slots:
    void onBtnColorClicked();
    void onBtnOkClicked();

private:
    QDoubleSpinBox*     m_spinBoxX;
    QDoubleSpinBox*     m_spinBoxY;
    QDoubleSpinBox*     m_spinBoxZ;
    
    QDoubleSpinBox*     m_spinBoxRadius;
    QDoubleSpinBox*     m_spinBoxHeight;

    QColor              m_color;
    QPushButton*        m_btnColor;
};
