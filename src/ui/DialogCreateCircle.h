#pragma once

#include <QDialog>
#include <QColor>
#include <QVector>

#include <gp_Pnt.hxx>

class QDoubleSpinBox;
class QPushButton;
class QLabel;
class ShapePickSession;

/**
 * @class DialogCreateCircle
 * @brief Dialog for interactively picking two 3-D points to define a circle in the XY plane.
 *
 * Uses a ShapePickSession (requiredPointCount = 2):
 * - First click sets the centre point; the circle preview follows the mouse,
 *   its radius equal to the distance between the centre and the cursor.
 * - Second click fixes the radius; the Center and Radius SpinBoxes are auto-populated.
 */
class DialogCreateCircle : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateCircle(QWidget* parent = nullptr);
    ~DialogCreateCircle() override;

    void show();

    double x() const;
    double y() const;
    double z() const;

    double radius() const;

    QColor color() const;

signals:
    void signalCreateCircle(double x, double y, double z,
                            double radius,
                            const QColor& color);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onBtnColorClicked();
    void onBtnOkClicked();
    void onSessionCompleted(QVector<gp_Pnt> points);

private:
    QDoubleSpinBox*   m_spinBoxX{nullptr};
    QDoubleSpinBox*   m_spinBoxY{nullptr};
    QDoubleSpinBox*   m_spinBoxZ{nullptr};

    QDoubleSpinBox*   m_spinBoxRadius{nullptr};

    QColor            m_color{Qt::white};
    QPushButton*      m_btnColor{nullptr};
    QLabel*           m_statusLabel{nullptr};
    ShapePickSession* m_session{nullptr};
};
