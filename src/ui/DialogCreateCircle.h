#pragma once

#include <QDialog>
#include <QColor>

class QDoubleSpinBox;
class QPushButton;

class DialogCreateCircle : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateCircle(QWidget* parent = nullptr);
    ~DialogCreateCircle() override;

    double x() const;
    double y() const;
    double z() const;

    double radius() const;

    QColor color() const;

signals:
    void signalCreateCircle(double x, double y, double z, double radius, const QColor& color);

private slots:
    void onBtnColorClicked();
    void onBtnOkClicked();

private:
    QDoubleSpinBox*     m_spinBoxX;
    QDoubleSpinBox*     m_spinBoxY;
    QDoubleSpinBox*     m_spinBoxZ;

    QDoubleSpinBox*     m_spinBoxRadius;

    QColor              m_color;
    QPushButton*        m_btnColor;
};
