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
 * @class DialogCreateEllipse
 * @brief Dialog for interactively picking two 3-D points to define an ellipse.
 *
 * Uses a ShapePickSession (requiredPointCount = 2):
 * - First click sets the centre point; the ellipse preview follows the mouse,
 *   its major radius equal to dist(centre, cursor) and minor radius = majorRadius / 2.
 * - Second click fixes the geometry; SpinBoxes are auto-populated.
 *
 * The plane normal is taken from the Normal Direction SpinBoxes (default: Z-axis).
 */
class DialogCreateEllipse : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateEllipse(QWidget* parent = nullptr);
    ~DialogCreateEllipse() override;

    void show();

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
    void signalCreateEllipse(double cx, double cy, double cz,
                             double nx, double ny, double nz,
                             double major, double minor,
                             const QColor& color);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onBtnColorClicked();
    void onBtnOkClicked();
    void onSessionCompleted(QVector<gp_Pnt> points);

private:
    QDoubleSpinBox*   m_spinBoxCenterX{nullptr};
    QDoubleSpinBox*   m_spinBoxCenterY{nullptr};
    QDoubleSpinBox*   m_spinBoxCenterZ{nullptr};

    QDoubleSpinBox*   m_spinBoxNormalX{nullptr};
    QDoubleSpinBox*   m_spinBoxNormalY{nullptr};
    QDoubleSpinBox*   m_spinBoxNormalZ{nullptr};

    QDoubleSpinBox*   m_spinBoxMajorRadius{nullptr};
    QDoubleSpinBox*   m_spinBoxMinorRadius{nullptr};

    QColor            m_color{Qt::white};
    QPushButton*      m_btnColor{nullptr};
    QLabel*           m_statusLabel{nullptr};
    ShapePickSession* m_session{nullptr};
};
