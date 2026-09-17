#pragma once

#include <QDialog>
#include <QColor>
#include <QVector>

#include <gp_Pnt.hxx>

class QDoubleSpinBox;
class QPushButton;
class QLabel;
class ShapePickSession;

class DialogCreateParabola : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCreateParabola(QWidget* parent = nullptr);
    ~DialogCreateParabola() override;

    void show();
    void done(int result) override;

    double vertexX() const;
    double vertexY() const;
    double vertexZ() const;

    double normalX() const;
    double normalY() const;
    double normalZ() const;

    double focalLength() const;
    double firstParameter() const;
    double lastParameter() const;

    QColor color() const;

signals:
    void signalCreateParabola(double vx, double vy, double vz,
                             double nx, double ny, double nz,
                             double focalLength, double firstParameter, double lastParameter,
                             const QColor& color);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onBtnColorClicked();
    void onBtnOkClicked();
    void onSessionCompleted(QVector<gp_Pnt> points);

private:
    QDoubleSpinBox*   m_spinBoxVertexX{nullptr};
    QDoubleSpinBox*   m_spinBoxVertexY{nullptr};
    QDoubleSpinBox*   m_spinBoxVertexZ{nullptr};

    QDoubleSpinBox*   m_spinBoxNormalX{nullptr};
    QDoubleSpinBox*   m_spinBoxNormalY{nullptr};
    QDoubleSpinBox*   m_spinBoxNormalZ{nullptr};

    QDoubleSpinBox*   m_spinBoxFocalLength{nullptr};

    QDoubleSpinBox*   m_spinBoxFirstParameter{nullptr};
    QDoubleSpinBox*   m_spinBoxLastParameter{nullptr};

    QColor            m_color{Qt::white};
    QPushButton*      m_btnColor{nullptr};
    QLabel*           m_statusLabel{nullptr};
    ShapePickSession* m_session{nullptr};
};
