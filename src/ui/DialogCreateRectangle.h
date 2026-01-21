#pragma once

#include <QDialog>
#include <QColor>

class QDoubleSpinBox;
class QPushButton;

class DialogCreateRectangle : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateRectangle(QWidget* parent = nullptr);
    ~DialogCreateRectangle() override;

    double x() const;
    double y() const;
    double z() const;

    double width() const;
    double height() const;

    QColor color() const;

private slots:
    void onBtnColorClicked();

private:
    QDoubleSpinBox*     m_spinBoxX;
    QDoubleSpinBox*     m_spinBoxY;
    QDoubleSpinBox*     m_spinBoxZ;

    QDoubleSpinBox*     m_spinBoxWidth;
    QDoubleSpinBox*     m_spinBoxHeight;

    QColor              m_color;
    QPushButton*        m_btnColor;
};
