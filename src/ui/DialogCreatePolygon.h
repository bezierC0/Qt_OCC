#pragma once

#include <QDialog>
#include <QColor>
#include <QList>
#include <gp_Pnt.hxx>

class QDoubleSpinBox;
class QPushButton;
class QGroupBox;
class QVBoxLayout;
class QScrollArea;
class QCheckBox;

class DialogCreatePolygon : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreatePolygon(QWidget* parent = nullptr);
    ~DialogCreatePolygon() override;

    QList<gp_Pnt> points() const;
    bool isClosed() const;
    QColor color() const;

signals:
    void signalCreatePolygon(const QList<gp_Pnt>& points, bool isClosed, const QColor& color);

private slots:
    void onBtnAddPointClicked();
    void onBtnRemovePointClicked();
    void onBtnColorClicked();
    void onBtnOkClicked();

private:
    struct PointWidgets {
        QGroupBox* group;
        QDoubleSpinBox* spinX;
        QDoubleSpinBox* spinY;
        QDoubleSpinBox* spinZ;
    };

    void addPointRow(double x = 0, double y = 0, double z = 0);
    void removePointRow();

    QVBoxLayout*        m_pointsLayout;
    QList<PointWidgets> m_pointWidgetsList;
    
    QCheckBox*          m_checkClosed;
    QColor              m_color;
    QPushButton*        m_btnColor;
    QScrollArea*        m_scrollArea;
};
