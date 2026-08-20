#pragma once

#include <QDialog>
#include <QString>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QSpinBox;

class DialogCaeColorRange final : public QDialog {
    Q_OBJECT

public:
    DialogCaeColorRange(
        bool automatic,
        double minimum,
        double maximum,
        double dataMinimum,
        double dataMaximum,
        int bandCount,
        const QString& unit,
        QWidget* parent = nullptr);

    bool isAutomatic() const;
    double minimum() const;
    double maximum() const;
    int bandCount() const;

private slots:
    void updateInputState(bool automatic);
    void accept() override;

private:
    QCheckBox* m_automaticCheckBox{nullptr};
    QDoubleSpinBox* m_minimumSpinBox{nullptr};
    QDoubleSpinBox* m_maximumSpinBox{nullptr};
    QSpinBox* m_bandCountSpinBox{nullptr};
};
