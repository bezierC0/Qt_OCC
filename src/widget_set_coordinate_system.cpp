#include "widget_set_coordinate_system.h"
#include "ui_widget_set_coordinate_system.h"
#include "OCCView.h"
#include "ViewManager.h"

#include <QWindow>
#include <QMessageBox>
#include <QDebug>
#include <QDoubleSpinBox>

WidgetSetCoordinateSystem::WidgetSetCoordinateSystem(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetSetCoordinateSystem)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint );

    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &WidgetSetCoordinateSystem::onPushButtonCancel);
    connect(ui->coordinateEditPointX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetSetCoordinateSystem::onCoordinateChanged);
    connect(ui->coordinateEditPointY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetSetCoordinateSystem::onCoordinateChanged);
    connect(ui->coordinateEditPointZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetSetCoordinateSystem::onCoordinateChanged);
    connect(ui->coordinateEditNormalX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetSetCoordinateSystem::onCoordinateChanged);
    connect(ui->coordinateEditNormalY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetSetCoordinateSystem::onCoordinateChanged);
    connect(ui->coordinateEditNormalZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetSetCoordinateSystem::onCoordinateChanged);
}

WidgetSetCoordinateSystem::~WidgetSetCoordinateSystem()
{
    delete ui;
}

void WidgetSetCoordinateSystem::onPushButtonCancel()
{
    close();
}

void WidgetSetCoordinateSystem::createWorkPlane()
{
    auto view = ViewManager::getInstance().getActiveView();
    if(!view){
        QMessageBox::warning(this, tr("Error"), tr("No active view"));
        return;
    }
    view->createWorkPlane(ui->coordinateEditPointX->value(), 
                            ui->coordinateEditPointY->value(), 
                            ui->coordinateEditPointZ->value(),
                            ui->coordinateEditNormalX->value(), 
                            ui->coordinateEditNormalY->value(), 
                            ui->coordinateEditNormalZ->value());
}

void WidgetSetCoordinateSystem::onCoordinateChanged()
{
    qDebug() << "Coordinate changed!";
    createWorkPlane();
}
