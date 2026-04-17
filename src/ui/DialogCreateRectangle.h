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
 * @class DialogCreateRectangle
 * @brief Dialog for interactively picking two 3-D points to define a rectangle in the XY plane.
 *
 * Uses a ShapePickSession (requiredPointCount = 2):
 * - First click sets the origin corner; the rectangle outline follows the mouse in real time.
 * - Second click fixes the opposite corner; Width / Height SpinBoxes are auto-populated.
 */
class DialogCreateRectangle : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateRectangle(QWidget* parent = nullptr);
    ~DialogCreateRectangle() override;

    void show();

    double x() const;
    double y() const;
    double z() const;

    double width() const;
    double height() const;

    QColor color() const;

signals:
    void signalCreateRectangle(double x, double y, double z,
                               double width, double height,
                               const QColor& color);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onBtnOkClicked();
    void onBtnColorClicked();
    void onSessionCompleted(QVector<gp_Pnt> points);

private:
    QDoubleSpinBox*   m_spinBoxX{nullptr};
    QDoubleSpinBox*   m_spinBoxY{nullptr};
    QDoubleSpinBox*   m_spinBoxZ{nullptr};

    QDoubleSpinBox*   m_spinBoxWidth{nullptr};
    QDoubleSpinBox*   m_spinBoxHeight{nullptr};

    QColor            m_color{Qt::white};
    QPushButton*      m_btnColor{nullptr};
    QLabel*           m_statusLabel{nullptr};
    ShapePickSession* m_session{nullptr};
};
