#pragma once

#include <QWidget>

namespace Ui {
class WidgetSetCoordinateSystem;
}

class WidgetSetCoordinateSystem : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetSetCoordinateSystem(QWidget *parent = nullptr);
    ~WidgetSetCoordinateSystem();

private:
    Ui::WidgetSetCoordinateSystem *ui;
};
