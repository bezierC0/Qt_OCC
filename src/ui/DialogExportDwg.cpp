#include "DialogExportDwg.h"
#include "ui_DialogExportDwg.h"

#include <QFileDialog>
#include <QDir>

DialogExportDwg::DialogExportDwg(QWidget* parent)
    : QDialog(parent), ui(new Ui::DialogExportDwg)
{
    ui->setupUi(this);

    connect(ui->m_btnBrowseOda, &QPushButton::clicked, this, &DialogExportDwg::onBrowseOdaPath);
    connect(ui->m_btnBrowseOutput, &QPushButton::clicked, this, &DialogExportDwg::onBrowseOutputPath);
    connect(ui->m_btnExport, &QPushButton::clicked, this, &DialogExportDwg::onExportClicked);
    connect(ui->m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

DialogExportDwg::~DialogExportDwg()
{
    delete ui;
}

QString DialogExportDwg::odaExePath() const
{
    return QDir::fromNativeSeparators(ui->m_editOdaPath->text().trimmed());
}

QString DialogExportDwg::outputPath() const
{
    return QDir::fromNativeSeparators(ui->m_editOutputPath->text().trimmed());
}

void DialogExportDwg::setOdaExePath(const QString& path)
{
    ui->m_editOdaPath->setText(QDir::toNativeSeparators(path));
}

void DialogExportDwg::setOutputPath(const QString& path)
{
    ui->m_editOutputPath->setText(QDir::toNativeSeparators(path));
}

void DialogExportDwg::setStatusText(const QString& text)
{
    ui->m_lblStatus->setText(text);
}

void DialogExportDwg::setProgressValue(int value)
{
    ui->m_progressBar->setValue(value);
}

void DialogExportDwg::setExportEnabled(bool enabled)
{
    ui->m_btnExport->setEnabled(enabled);
}

void DialogExportDwg::onBrowseOdaPath()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Select ODAFileConverter.exe"),
        ui->m_editOdaPath->text(),
        tr("Executable (*.exe);;All files (*.*)"));
    if (!path.isEmpty()) {
        ui->m_editOdaPath->setText(QDir::toNativeSeparators(path));
    }
}

void DialogExportDwg::onBrowseOutputPath()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("Save as DWG File"),
        ui->m_editOutputPath->text(),
        tr("DWG Files (*.dwg)"));
    if (!path.isEmpty()) {
        ui->m_editOutputPath->setText(QDir::toNativeSeparators(path));
    }
}

void DialogExportDwg::onExportClicked()
{
    emit signalExportRequested();
}
