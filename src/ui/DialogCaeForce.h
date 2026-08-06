#pragma once

#include <QDialog>
#include <array>

class QDoubleSpinBox;

class DialogCaeForce final : public QDialog {
    Q_OBJECT

public:
    explicit DialogCaeForce(
        const std::array<double, 3>& initialForce,
        QWidget* parent = nullptr);

    std::array<double, 3> force() const;

private:
    std::array<QDoubleSpinBox*, 3> m_componentSpinBoxes{};
};
