#pragma once

#include <QDialog>
#include <QColor>

class QDoubleSpinBox;
class QPushButton;

class DialogCreateEllipse : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateEllipse(QWidget* parent = nullptr);
    ~DialogCreateEllipse() override;

    double centerX() const;
    double centerY() const;
    double centerZ() const;

    double normalX() const;
    double normalY() const;
    double normalZ() const;

    double majorRadius() const;
    double minorRadius() const;

    QColor color() const;

signals:
    void signalCreateEllipse(double cx, double cy, double cz, double nx, double ny, double nz, double major, double minor, const QColor& color);

private slots:
    void onBtnColorClicked();
    void onBtnOkClicked();

private:
    QDoubleSpinBox*     m_spinBoxCenterX;
    QDoubleSpinBox*     m_spinBoxCenterY;
    QDoubleSpinBox*     m_spinBoxCenterZ;

    QDoubleSpinBox*     m_spinBoxNormalX;
    QDoubleSpinBox*     m_spinBoxNormalY;
    QDoubleSpinBox*     m_spinBoxNormalZ;

    QDoubleSpinBox*     m_spinBoxMajorRadius;
    QDoubleSpinBox*     m_spinBoxMinorRadius;

    QColor              m_color;
    QPushButton*        m_btnColor;
};
