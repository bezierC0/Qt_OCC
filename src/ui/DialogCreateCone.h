#pragma once

#include <QDialog>
#include <QColor>

class QDoubleSpinBox;
class QPushButton;

class DialogCreateCone : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateCone(QWidget* parent = nullptr);
    ~DialogCreateCone() override;

    double x() const;
    double y() const;
    double z() const;
    double radius1() const;
    double radius2() const;
    double height() const;
    QColor color() const;

signals:
    void signalCreateCone(double x, double y, double z, double r1, double r2, double h, const QColor& color);

private slots:
    void onBtnColorClicked();
    void onBtnOkClicked();

private:
    QDoubleSpinBox*     m_spinBoxX;
    QDoubleSpinBox*     m_spinBoxY;
    QDoubleSpinBox*     m_spinBoxZ;
    
    QDoubleSpinBox*     m_spinBoxRadius1;
    QDoubleSpinBox*     m_spinBoxRadius2;
    QDoubleSpinBox*     m_spinBoxHeight;

    QColor              m_color;
    QPushButton*        m_btnColor;
};
