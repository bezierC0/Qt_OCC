#pragma once

#include <QDialog>
#include <QColor>

class QDoubleSpinBox;
class QPushButton;

class DialogCreateLine : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateLine(QWidget* parent = nullptr);
    ~DialogCreateLine() override;

    double x1() const;
    double y1() const;
    double z1() const;

    double x2() const;
    double y2() const;
    double z2() const;

    QColor color() const;

signals:
    void signalCreateLine(double x1, double y1, double z1, double x2, double y2, double z2, const QColor& color);

private slots:
    void onBtnColorClicked();
    void onBtnOkClicked();

private:
    QDoubleSpinBox*     m_spinBoxX1;
    QDoubleSpinBox*     m_spinBoxY1;
    QDoubleSpinBox*     m_spinBoxZ1;

    QDoubleSpinBox*     m_spinBoxX2;
    QDoubleSpinBox*     m_spinBoxY2;
    QDoubleSpinBox*     m_spinBoxZ2;

    QColor              m_color;
    QPushButton*        m_btnColor;
};
