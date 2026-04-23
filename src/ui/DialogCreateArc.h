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
 * @class DialogCreateArc
 * @brief Dialog for interactively picking three 3-D points to define a circular arc.
 *
 * Internally drives a ShapePickSession (requiredPointCount = 3):
 * - After the first click, Preview shows a reference line from P1 to the mouse cursor.
 * - After the second click, Preview shows the arc through P1, P2 and the mouse cursor.
 *   If the three points are collinear the preview degrades gracefully to a line segment.
 * - After the third click the session completes, SpinBoxes are populated, and the
 *   preview is cleared automatically.
 */
class DialogCreateArc : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateArc(QWidget* parent = nullptr);
    ~DialogCreateArc() override;

    double x1() const;
    double y1() const;
    double z1() const;

    double x2() const;
    double y2() const;
    double z2() const;

    double x3() const;
    double y3() const;
    double z3() const;

    QColor color() const;

signals:
    void signalCreateArc(double x1, double y1, double z1,
                         double x2, double y2, double z2,
                         double x3, double y3, double z3,
                         const QColor& color);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onBtnColorClicked();
    void onBtnOkClicked();

    /// Triggered by ShapePickSession::sessionCompleted.
    void onSessionCompleted(QVector<gp_Pnt> points);

private:
    QDoubleSpinBox*   m_spinBoxX1{nullptr};
    QDoubleSpinBox*   m_spinBoxY1{nullptr};
    QDoubleSpinBox*   m_spinBoxZ1{nullptr};

    QDoubleSpinBox*   m_spinBoxX2{nullptr};
    QDoubleSpinBox*   m_spinBoxY2{nullptr};
    QDoubleSpinBox*   m_spinBoxZ2{nullptr};

    QDoubleSpinBox*   m_spinBoxX3{nullptr};
    QDoubleSpinBox*   m_spinBoxY3{nullptr};
    QDoubleSpinBox*   m_spinBoxZ3{nullptr};

    QColor            m_color{Qt::white};
    QPushButton*      m_btnColor{nullptr};
    QLabel*           m_statusLabel{nullptr};   ///< Shows the current picking-step hint.
    ShapePickSession* m_session{nullptr};
};
