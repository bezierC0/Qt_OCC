#pragma once

#include <QWidget>
#include <QCloseEvent>

namespace Ui {
class WidgetClipping;
}

class WidgetClipping : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetClipping(QWidget *parent = nullptr);
    ~WidgetClipping();
    void closeEvent(QCloseEvent *event) override;

public Q_SLOTS:
    void show();
    void hide();

private slots:
    void onCoordinateChanged();
    void onClippingChanged();
    void onCappingChanged();
    void onPushButtonCancel();

private:
    void addClippingPlane();

private:
    Ui::WidgetClipping *ui;
};
