#pragma once

#include <QDialog>

#include <QDialog>
#include <QColor>

class QDoubleSpinBox;
class QPushButton;

class DialogCreatePoint : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreatePoint(QWidget *parent = nullptr);
    ~DialogCreatePoint() override;

    double x() const;
    double y() const;
    double z() const;
    QColor color() const;

signals:
    void signalCreatePoint(double x, double y, double z, const QColor& color);

private slots:
    void onBtnOkClicked();
    void onBtnColorClicked();

private:
    QDoubleSpinBox*     m_spinBoxX;
    QDoubleSpinBox*     m_spinBoxY;
    QDoubleSpinBox*     m_spinBoxZ;
    QColor              m_color;
    QPushButton*        m_btnColor;
};
