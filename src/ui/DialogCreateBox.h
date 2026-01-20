#pragma once

#include <QDialog>

#include <QDialog>
#include <QColor>

class QDoubleSpinBox;
class QPushButton;

class DialogCreateBox : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateBox(QWidget *parent = nullptr);
    ~DialogCreateBox() override;

    double x() const;
    double y() const;
    double z() const;
    double dx() const;
    double dy() const;
    double dz() const;
    QColor color() const;

private slots:
    void onBtnColorClicked();

private:
    QDoubleSpinBox*     m_spinBoxX;
    QDoubleSpinBox*     m_spinBoxY;
    QDoubleSpinBox*     m_spinBoxZ;
    QDoubleSpinBox*     m_spinBoxDX;
    QDoubleSpinBox*     m_spinBoxDY;
    QDoubleSpinBox*     m_spinBoxDZ;
    QColor              m_color;
    QPushButton*        m_btnColor;
};
