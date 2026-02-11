#include "DialogCreatePolygon.h"
#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QColorDialog>
#include <QGroupBox>
#include <QScrollArea>
#include <QCheckBox>

DialogCreatePolygon::DialogCreatePolygon(QWidget *parent) : QDialog(parent), m_color(Qt::white)
{
    setWindowTitle("Create Polygon");
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setWindowIcon(QIcon());

    auto* mainLayout = new QVBoxLayout(this);
    
    // Scroll Area for Points
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setMinimumHeight(300);
    m_scrollArea->setMinimumWidth(300);
    
    auto* scrollWidget = new QWidget(m_scrollArea);
    m_pointsLayout = new QVBoxLayout(scrollWidget);
    m_pointsLayout->setAlignment(Qt::AlignTop);
    
    m_scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(m_scrollArea);

    // Initial 2 points
    addPointRow(0, 0, 0);
    addPointRow(20, 10, 0);

    // Control Buttons (Add/Remove)
    auto* ctrlLayout = new QHBoxLayout();
    auto* btnAdd = new QPushButton("Add Point", this);
    auto* btnRemove = new QPushButton("Remove Point", this);
    connect(btnAdd, &QPushButton::clicked, this, &DialogCreatePolygon::onBtnAddPointClicked);
    connect(btnRemove, &QPushButton::clicked, this, &DialogCreatePolygon::onBtnRemovePointClicked);
    ctrlLayout->addWidget(btnAdd);
    ctrlLayout->addWidget(btnRemove);
    mainLayout->addLayout(ctrlLayout);

    // Options
    auto* optionsLayout = new QHBoxLayout();
    m_checkClosed = new QCheckBox("Closed Polygon", this);
    m_checkClosed->setChecked(true);
    optionsLayout->addWidget(m_checkClosed);
    
    m_btnColor = new QPushButton("Select Color", this);
    connect(m_btnColor, &QPushButton::clicked, this, &DialogCreatePolygon::onBtnColorClicked);
    optionsLayout->addWidget(new QLabel("Color:"));
    optionsLayout->addWidget(m_btnColor);
    mainLayout->addLayout(optionsLayout);

    // OK/Cancel Buttons
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Create");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DialogCreatePolygon::onBtnOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

DialogCreatePolygon::~DialogCreatePolygon()
{
}

QList<gp_Pnt> DialogCreatePolygon::points() const
{
    QList<gp_Pnt> pts;
    for (const auto& w : m_pointWidgetsList) {
        pts.append(gp_Pnt(w.spinX->value(), w.spinY->value(), w.spinZ->value()));
    }
    return pts;
}

bool DialogCreatePolygon::isClosed() const
{
    return m_checkClosed->isChecked();
}

QColor DialogCreatePolygon::color() const
{
    return m_color;
}

void DialogCreatePolygon::onBtnAddPointClicked()
{
    addPointRow();
}

void DialogCreatePolygon::onBtnRemovePointClicked()
{
    if (m_pointWidgetsList.size() > 2) {
        removePointRow();
    }
}

void DialogCreatePolygon::onBtnColorClicked()
{
    const QColor c = QColorDialog::getColor(m_color, this, "Select Color");
    if (c.isValid()) {
        m_color = c;
        QString style = QString("background-color: %1").arg(c.name());
        m_btnColor->setStyleSheet(style);
    }
}

void DialogCreatePolygon::onBtnOkClicked()
{
    emit signalCreatePolygon(points(), isClosed(), color());
}

void DialogCreatePolygon::addPointRow(double x, double y, double z)
{
    PointWidgets w;
    w.group = new QGroupBox(QString("Point %1").arg(m_pointWidgetsList.size() + 1), this);
    auto* formLayout = new QFormLayout(w.group);

    w.spinX = new QDoubleSpinBox(this);
    w.spinX->setRange(-10000.0, 10000.0);
    w.spinX->setValue(x);

    w.spinY = new QDoubleSpinBox(this);
    w.spinY->setRange(-10000.0, 10000.0);
    w.spinY->setValue(y);

    w.spinZ = new QDoubleSpinBox(this);
    w.spinZ->setRange(-10000.0, 10000.0);
    w.spinZ->setValue(z);

    formLayout->addRow("X:", w.spinX);
    formLayout->addRow("Y:", w.spinY);
    formLayout->addRow("Z:", w.spinZ);

    m_pointsLayout->addWidget(w.group);
    m_pointWidgetsList.append(w);
}

void DialogCreatePolygon::removePointRow()
{
    if (m_pointWidgetsList.isEmpty()) return;
    
    PointWidgets w = m_pointWidgetsList.takeLast();
    m_pointsLayout->removeWidget(w.group);
    w.group->deleteLater();
}
