#include "widget_explode_assembly.h"
#include "ui_widget_explode_assembly.h"
#include "ViewManager.h"
#include "OCCView.h"

WidgetExplodeAssembly::WidgetExplodeAssembly(QWidget* parent)
    : QWidget(parent),
      m_ui(new Ui_WidgetExplodeAssembly)
{
    m_ui->setupUi(this);

    QObject::connect(m_ui->slider_Factor, &QSlider::valueChanged, this, &WidgetExplodeAssembly::onSliderValueChanged);
    QObject::connect(m_ui->edit_Factor, qOverload<int>(&QSpinBox::valueChanged), this, &WidgetExplodeAssembly::onSpinBoxValueChanged);
}

WidgetExplodeAssembly::~WidgetExplodeAssembly()
{
    delete m_ui;
}

void WidgetExplodeAssembly::onSliderValueChanged(int value)
{
    m_explodeValue = value;
    QSignalBlocker sigBlock(m_ui->edit_Factor);
    m_ui->edit_Factor->setValue(m_explodeValue);
    explode();
}

void WidgetExplodeAssembly::onSpinBoxValueChanged(int value)
{
    m_explodeValue = value;
    QSignalBlocker sigBlock(m_ui->slider_Factor);
    m_ui->slider_Factor->setValue(value);
    explode();
}

void WidgetExplodeAssembly::explode()
{
    static constexpr double factor = 1.0 / 100.0;
    if (const auto activeView = ViewManager::getInstance().getActiveView()) {
        activeView->explosion(m_explodeValue * factor);
    }
}
