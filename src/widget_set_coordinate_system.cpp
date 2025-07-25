#include "widget_set_coordinate_system.h"
#include "ui_widget_set_coordinate_system.h"

WidgetSetCoordinateSystem::WidgetSetCoordinateSystem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetSetCoordinateSystem)
{
    ui->setupUi(this);
}

WidgetSetCoordinateSystem::~WidgetSetCoordinateSystem()
{
    delete ui;
}
