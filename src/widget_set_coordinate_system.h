#pragma once

#include <QWidget>
#include <QCloseEvent>

namespace Ui {
class WidgetSetCoordinateSystem;
}

class WidgetSetCoordinateSystem : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetSetCoordinateSystem(QWidget *parent = nullptr);
    ~WidgetSetCoordinateSystem();
    void closeEvent(QCloseEvent *event) override;

public Q_SLOTS:
    void show();
    void hide();

private slots:
    void onCoordinateChanged();
    void onPushButtonCancel();

private:
    void createWorkPlane();

private:
    Ui::WidgetSetCoordinateSystem *ui;
};
