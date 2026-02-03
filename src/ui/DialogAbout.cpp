#include "DialogAbout.h"
#include "ui_dialog_about.h"

DialogAbout::DialogAbout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogAbout)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("About"));
    ui->label->setText(tr("Qt_OCC Application"));
    ui->label_2->setText(tr("Version 1.0.0"));
}

DialogAbout::~DialogAbout()
{
    delete ui;
}
