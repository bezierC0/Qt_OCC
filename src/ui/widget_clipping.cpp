#include "widget_clipping.h"
#include "ui_widget_clipping.h"
#include "OCCView.h"
#include "ViewManager.h"

#include <QWindow>
#include <QMessageBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QCheckBox>

WidgetClipping::WidgetClipping(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetClipping)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint );

    connect(ui->pushButtonCancel, &QPushButton::clicked, this, &WidgetClipping::onPushButtonCancel);
    connect(ui->coordinateEditPointX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetClipping::onCoordinateChanged);
    connect(ui->coordinateEditPointY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetClipping::onCoordinateChanged);
    connect(ui->coordinateEditPointZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetClipping::onCoordinateChanged);
    connect(ui->coordinateEditNormalX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetClipping::onCoordinateChanged);
    connect(ui->coordinateEditNormalY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetClipping::onCoordinateChanged);
    connect(ui->coordinateEditNormalZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetClipping::onCoordinateChanged);

    connect(ui->checkIsClipping, &QCheckBox::stateChanged, this, &WidgetClipping::onClippingChanged);
    connect(ui->checkIsCapping, &QCheckBox::stateChanged, this, &WidgetClipping::onCappingChanged);
}

WidgetClipping::~WidgetClipping()
{
    delete ui;
}

void WidgetClipping::closeEvent(QCloseEvent *event)
{
    auto view = ViewManager::getInstance().getActiveView();
    if(view){
        view->deactivateWorkPlane();
    }
    QWidget::closeEvent(event);
}

void WidgetClipping::show()
{
    QWidget::show();
    addClippingPlane();
}

void WidgetClipping::hide()
{
    auto view = ViewManager::getInstance().getActiveView();
    if(view){
        view->deactivateWorkPlane();
    }
    QWidget::hide();
}

void WidgetClipping::onClippingChanged()
{
    auto view = ViewManager::getInstance().getActiveView();
    if(!view){
        QMessageBox::warning(this, tr("Error"), tr("No active view"));
        return;
    }
    view->setClippingPlaneIsOn(ui->checkIsClipping->isChecked()) ;
}

void WidgetClipping::onCappingChanged()
{
    auto view = ViewManager::getInstance().getActiveView();
    if(!view){
        QMessageBox::warning(this, tr("Error"), tr("No active view"));
        return;
    }
    view->setCappingPlaneIsCap(ui->checkIsCapping->isChecked()) ;
}

void WidgetClipping::onPushButtonCancel()
{
    auto view = ViewManager::getInstance().getActiveView();
    if(!view){
        QMessageBox::warning(this, tr("Error"), tr("No active view"));
        return;
    }
    view->setClippingPlaneIsOn(false) ;
    hide();
}

void WidgetClipping::addClippingPlane()
{
    auto view = ViewManager::getInstance().getActiveView();
    if(!view){
        QMessageBox::warning(this, tr("Error"), tr("No active view"));
        return;
    }
    const gp_Pnt point(ui->coordinateEditPointX->value(), ui->coordinateEditPointY->value(), ui->coordinateEditPointZ->value());
    auto isValidDirection =[](double dx, double dy, double dz) {
        try {
            gp_Dir dir(dx, dy, dz);
            return true;
        }
        catch (const Standard_ConstructionError&) {
            return false;
        };
    };
    if(!isValidDirection(ui->coordinateEditNormalX->value(), ui->coordinateEditNormalY->value(), ui->coordinateEditNormalZ->value())){
        return;
    }
    const gp_Dir normal(ui->coordinateEditNormalX->value(), ui->coordinateEditNormalY->value(), ui->coordinateEditNormalZ->value());
    bool isClipping = ui->checkIsClipping->isChecked();
    bool isCapping = ui->checkIsCapping->isChecked();
    view->addClippingPlane(point,normal,isClipping,isCapping);
}

void WidgetClipping::onCoordinateChanged()
{
    addClippingPlane();
}
