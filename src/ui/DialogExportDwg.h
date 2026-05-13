#pragma once

#include <QDialog>

namespace Ui {
class DialogExportDwg;
}

class DialogExportDwg : public QDialog
{
    Q_OBJECT

public:
    explicit DialogExportDwg(QWidget* parent = nullptr);
    ~DialogExportDwg() override;

    QString odaExePath() const;
    QString outputPath() const;

    void setOdaExePath(const QString& path);
    void setOutputPath(const QString& path);
    void setStatusText(const QString& text);
    void setProgressValue(int value);
    void setExportEnabled(bool enabled);

signals:
    void signalExportRequested();

private slots:
    void onBrowseOdaPath();
    void onBrowseOutputPath();
    void onExportClicked();

private:
    Ui::DialogExportDwg* ui;
};
