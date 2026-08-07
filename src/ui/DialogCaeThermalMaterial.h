#pragma once

#include <QDialog>

class QDoubleSpinBox;
class QLineEdit;

class DialogCaeThermalMaterial final : public QDialog {
    Q_OBJECT

public:
    explicit DialogCaeThermalMaterial(QWidget* parent = nullptr);
    DialogCaeThermalMaterial(
        const QString& name,
        double thermalConductivity,
        QWidget* parent = nullptr);

    QString materialName() const;
    double thermalConductivity() const;

private:
    QLineEdit* m_nameEdit{nullptr};
    QDoubleSpinBox* m_thermalConductivitySpinBox{nullptr};
};
