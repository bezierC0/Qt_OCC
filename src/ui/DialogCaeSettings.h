#pragma once

#include "cae/CaeExternalToolConfig.h"

#include <QDialog>

class QLineEdit;
class QSpinBox;

class DialogCaeSettings final : public QDialog {
    Q_OBJECT

public:
    explicit DialogCaeSettings(const Cae::CaeExternalToolConfig& config, QWidget* parent = nullptr);

    Cae::CaeExternalToolConfig config() const;

private:
    void browseExecutable(QLineEdit* targetLineEdit, const QString& title);
    void browseWorkingDirectory();

private:
    QLineEdit* m_gmshPathEdit{nullptr};
    QLineEdit* m_calculixPathEdit{nullptr};
    QLineEdit* m_getDpPathEdit{nullptr};
    QLineEdit* m_workingDirectoryEdit{nullptr};
    QSpinBox* m_timeoutSpinBox{nullptr};
};
