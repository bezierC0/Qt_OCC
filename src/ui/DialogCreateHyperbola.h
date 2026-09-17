#pragma once

#include <QDialog>
#include <QColor>
#include <QVector>

#include <gp_Pnt.hxx>

class QDoubleSpinBox;
class QPushButton;
class QLabel;
class ShapePickSession;

class DialogCreateHyperbola : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateHyperbola(QWidget* parent = nullptr);
    ~DialogCreateHyperbola() override;

    void show();
    void done(int result) override;

    double centerX() const;
    double centerY() const;
    double centerZ() const;

    double normalX() const;
    double normalY() const;
    double normalZ() const;

    double majorRadius() const;
    double minorRadius() const;
    double firstParameter() const;
    double lastParameter() const;

    QColor color() const;

signals:
    void signalCreateHyperbola(double cx, double cy, double cz,
                             double nx, double ny, double nz,
                             double major, double minor, double firstParameter, double lastParameter,
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

    QDoubleSpinBox*   m_spinBoxFirstParameter{nullptr};
    QDoubleSpinBox*   m_spinBoxLastParameter{nullptr};

    QColor            m_color{Qt::white};
    QPushButton*      m_btnColor{nullptr};
    QLabel*           m_statusLabel{nullptr};
    ShapePickSession* m_session{nullptr};
};
