#pragma once

#include <QDialog>
#include <QColor>

class QDoubleSpinBox;
class QPushButton;

class DialogCreateArc : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateArc(QWidget* parent = nullptr);
    ~DialogCreateArc() override;

    double x1() const;
    double y1() const;
    double z1() const;

    double x2() const;
    double y2() const;
    double z2() const;

    double x3() const;
    double y3() const;
    double z3() const;

    QColor color() const;

private slots:
    void onBtnColorClicked();

private:
    QDoubleSpinBox*     m_spinBoxX1;
    QDoubleSpinBox*     m_spinBoxY1;
    QDoubleSpinBox*     m_spinBoxZ1;

    QDoubleSpinBox*     m_spinBoxX2;
    QDoubleSpinBox*     m_spinBoxY2;
    QDoubleSpinBox*     m_spinBoxZ2;

    QDoubleSpinBox*     m_spinBoxX3;
    QDoubleSpinBox*     m_spinBoxY3;
    QDoubleSpinBox*     m_spinBoxZ3;

    QColor              m_color;
    QPushButton*        m_btnColor;
};
