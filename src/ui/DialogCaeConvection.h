#pragma once

#include <QDialog>

class QDoubleSpinBox;

class DialogCaeConvection final : public QDialog {
    Q_OBJECT

public:
    explicit DialogCaeConvection(QWidget* parent = nullptr);
    DialogCaeConvection(
        double filmCoefficient,
        double ambientTemperature,
        QWidget* parent = nullptr);

    double filmCoefficient() const;
    double ambientTemperature() const;

private:
    QDoubleSpinBox* m_filmCoefficientSpinBox{nullptr};
    QDoubleSpinBox* m_ambientTemperatureSpinBox{nullptr};
};
