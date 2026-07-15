#pragma once

#include <QDialog>

class QDoubleSpinBox;
class QLineEdit;

class DialogCaeMaterial final : public QDialog {
    Q_OBJECT

public:
    explicit DialogCaeMaterial(QWidget* parent = nullptr);

    QString materialName() const;
    double youngModulus() const;
    double poissonRatio() const;

private:
    QLineEdit* m_nameEdit{nullptr};
    QDoubleSpinBox* m_youngModulusSpinBox{nullptr};
    QDoubleSpinBox* m_poissonRatioSpinBox{nullptr};
};
