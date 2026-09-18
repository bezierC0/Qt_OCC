#pragma once

#include <QDialog>
#include <QColor>

class QDoubleSpinBox;
class QPushButton;

class DialogCreateTorus : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateTorus(QWidget* parent = nullptr);
    ~DialogCreateTorus() override;

    double x() const;
    double y() const;
    double z() const;
    double majorRadius() const;
    double minorRadius() const;
    QColor color() const;

signals:
    void signalCreateTorus(double x, double y, double z, double majorRadius, double minorRadius, const QColor& color);

private slots:
    void onBtnOkClicked();
    void onBtnColorClicked();

private:
    QDoubleSpinBox*     m_spinBoxX;
    QDoubleSpinBox*     m_spinBoxY;
    QDoubleSpinBox*     m_spinBoxZ;
    
    QDoubleSpinBox*     m_spinBoxMajorRadius;
    QDoubleSpinBox*     m_spinBoxMinorRadius;

    QColor              m_color;
    QPushButton*        m_btnColor;
};
