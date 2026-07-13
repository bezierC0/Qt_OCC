#include "DialogCaeSettings.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

QWidget* createPathEditor(QLineEdit* lineEdit, QPushButton* browseButton)
{
    auto* container = new QWidget;
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(lineEdit);
    layout->addWidget(browseButton);
    return container;
}

} // namespace

DialogCaeSettings::DialogCaeSettings(const Cae::CaeExternalToolConfig& config, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("CAE Settings"));

    m_gmshPathEdit = new QLineEdit(config.executablePath(Cae::ExternalTool::Gmsh), this);
    m_calculixPathEdit = new QLineEdit(config.executablePath(Cae::ExternalTool::CalculiX), this);
    m_getDpPathEdit = new QLineEdit(config.executablePath(Cae::ExternalTool::GetDP), this);
    m_workingDirectoryEdit = new QLineEdit(config.workingDirectory(), this);
    m_timeoutSpinBox = new QSpinBox(this);
    m_timeoutSpinBox->setRange(1000, 3600000);
    m_timeoutSpinBox->setSingleStep(1000);
    m_timeoutSpinBox->setValue(config.timeoutMilliseconds());
    m_timeoutSpinBox->setSuffix(tr(" ms"));

    auto* gmshBrowseButton = new QPushButton(tr("Browse"), this);
    auto* calculixBrowseButton = new QPushButton(tr("Browse"), this);
    auto* getDpBrowseButton = new QPushButton(tr("Browse"), this);
    auto* workingDirectoryBrowseButton = new QPushButton(tr("Browse"), this);

    connect(gmshBrowseButton, &QPushButton::clicked, this, [this]() {
        browseExecutable(m_gmshPathEdit, tr("Select gmsh.exe"));
    });
    connect(calculixBrowseButton, &QPushButton::clicked, this, [this]() {
        browseExecutable(m_calculixPathEdit, tr("Select ccx.exe"));
    });
    connect(getDpBrowseButton, &QPushButton::clicked, this, [this]() {
        browseExecutable(m_getDpPathEdit, tr("Select getdp.exe"));
    });
    connect(workingDirectoryBrowseButton, &QPushButton::clicked, this, &DialogCaeSettings::browseWorkingDirectory);

    auto* formLayout = new QFormLayout;
    formLayout->addRow(tr("Gmsh executable"), createPathEditor(m_gmshPathEdit, gmshBrowseButton));
    formLayout->addRow(tr("CalculiX executable"), createPathEditor(m_calculixPathEdit, calculixBrowseButton));
    formLayout->addRow(tr("GetDP executable"), createPathEditor(m_getDpPathEdit, getDpBrowseButton));
    formLayout->addRow(tr("Working directory"), createPathEditor(m_workingDirectoryEdit, workingDirectoryBrowseButton));
    formLayout->addRow(tr("Process timeout"), m_timeoutSpinBox);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCaeSettings::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DialogCaeSettings::reject);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

Cae::CaeExternalToolConfig DialogCaeSettings::config() const
{
    Cae::CaeExternalToolConfig config;
    config.setExecutablePath(Cae::ExternalTool::Gmsh, m_gmshPathEdit->text().trimmed());
    config.setExecutablePath(Cae::ExternalTool::CalculiX, m_calculixPathEdit->text().trimmed());
    config.setExecutablePath(Cae::ExternalTool::GetDP, m_getDpPathEdit->text().trimmed());
    config.setWorkingDirectory(m_workingDirectoryEdit->text().trimmed());
    config.setTimeoutMilliseconds(m_timeoutSpinBox->value());
    return config;
}

void DialogCaeSettings::browseExecutable(QLineEdit* targetLineEdit, const QString& title)
{
    const QString filePath = QFileDialog::getOpenFileName(this, title, targetLineEdit->text(), tr("Executable (*.exe);;All Files (*.*)"));
    if (!filePath.isEmpty()) {
        targetLineEdit->setText(filePath);
    }
}

void DialogCaeSettings::browseWorkingDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Select CAE working directory"), m_workingDirectoryEdit->text());
    if (!directory.isEmpty()) {
        m_workingDirectoryEdit->setText(directory);
    }
}
