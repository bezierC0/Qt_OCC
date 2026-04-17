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
 * @class DialogCreateLine
 * @brief Dialog for interactively picking two 3-D points to define a line segment.
 *
 * Internally drives a ShapePickSession (requiredPointCount = 2):
 * - After the first click the session enters Preview state and the line segment
 *   follows the mouse cursor in real time.
 * - After the second click the session completes, the SpinBoxes are populated,
 *   and the preview is cleared automatically.
 */
class DialogCreateLine : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateLine(QWidget* parent = nullptr);
    ~DialogCreateLine() override;

    void show();

    double x1() const;
    double y1() const;
    double z1() const;

    double x2() const;
    double y2() const;
    double z2() const;

    QColor color() const;

signals:
    void signalCreateLine(double x1, double y1, double z1,
                          double x2, double y2, double z2,
                          const QColor& color);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onBtnColorClicked();
    void onBtnOkClicked();

    /// Triggered by ShapePickSession::sessionCompleted; receives all confirmed points.
    void onSessionCompleted(QVector<gp_Pnt> points);

private:
    QDoubleSpinBox*   m_spinBoxX1{nullptr};
    QDoubleSpinBox*   m_spinBoxY1{nullptr};
    QDoubleSpinBox*   m_spinBoxZ1{nullptr};

    QDoubleSpinBox*   m_spinBoxX2{nullptr};
    QDoubleSpinBox*   m_spinBoxY2{nullptr};
    QDoubleSpinBox*   m_spinBoxZ2{nullptr};

    QColor            m_color{Qt::white};
    QPushButton*      m_btnColor{nullptr};
    QLabel*           m_statusLabel{nullptr};   ///< Shows the current picking-step hint.
    ShapePickSession* m_session{nullptr};
};
