#pragma once

#include <QDialog>

#include <QDialog>
#include <QColor>

class QDoubleSpinBox;
class QPushButton;
class ViewerPickHelper;

class DialogCreatePoint : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreatePoint(QWidget *parent = nullptr);
    ~DialogCreatePoint() override;

    void show();

    double x() const;
    double y() const;
    double z() const;
    QColor color() const;

signals:
    void signalCreatePoint(double x, double y, double z, const QColor& color);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onBtnOkClicked();
    void onBtnColorClicked();
    void onPointPicked(double x, double y, double z);

private:
    QDoubleSpinBox*     m_spinBoxX;
    QDoubleSpinBox*     m_spinBoxY;
    QDoubleSpinBox*     m_spinBoxZ;
    QColor              m_color;
    QPushButton*        m_btnColor;
    ViewerPickHelper*   m_pickHelper{nullptr};
};
