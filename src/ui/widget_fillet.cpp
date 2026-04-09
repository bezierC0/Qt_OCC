#include "widget_fillet.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include "ViewManager.h"
#include "OCCView.h"
#include "SelectedEntity.h"
#include <BRep_Tool.hxx>

WidgetFillet::WidgetFillet(QWidget* parent)
    : QWidget(parent), m_isPicking(false)
{
    setWindowTitle("Fillet Feature");
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    auto* mainLayout = new QVBoxLayout(this);
    
    lblStatus = new QLabel("Ready to pick an edge.", this);
    mainLayout->addWidget(lblStatus);

    btnPick = new QPushButton("Pick Edge", this);
    mainLayout->addWidget(btnPick);

    auto* formLayout = new QFormLayout();
    spinRadius = new QDoubleSpinBox(this);
    spinRadius->setRange(0.01, 1000.0);
    spinRadius->setValue(1.0);
    
    formLayout->addRow("Radius:", spinRadius);
    mainLayout->addLayout(formLayout);

    btnApply = new QPushButton("Apply Fillet", this);
    mainLayout->addWidget(btnApply);

    connect(btnPick, &QPushButton::clicked, this, &WidgetFillet::onPickClicked);
    connect(btnApply, &QPushButton::clicked, this, &WidgetFillet::onApplyClicked);
}

WidgetFillet::~WidgetFillet()
{
}

void WidgetFillet::show()
{
    QWidget::show();
}

void WidgetFillet::hide()
{
    if (m_isPicking) {
        restoreMouseState();
    }
    QWidget::hide();
}

void WidgetFillet::closeEvent(QCloseEvent* event)
{
    if (m_isPicking) {
        restoreMouseState();
    }
    QWidget::closeEvent(event);
}

void WidgetFillet::onPickClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    if (m_isPicking) return;

    saveMouseState();
    m_isPicking = true;

    view->clearSelectedObjects();
    view->updateSelectionFilter(TopAbs_EDGE, true);
    view->setMouseMode(View::MouseMode::SELECTION);

    connect(view, &OCCView::signalSpaceSelected, this, &WidgetFillet::onObjectSelected);
    lblStatus->setText("Please select an edge.");
}

void WidgetFillet::onObjectSelected(const TopoDS_Shape& shape)
{
    if (!m_isPicking || shape.IsNull()) return;
    if (shape.ShapeType() != TopAbs_EDGE) return;

    m_selectedEdge = shape;
    lblStatus->setText("Edge picked. Ready to fillet.");
    restoreMouseState(); // Stop picking after one edge
}

void WidgetFillet::onApplyClicked()
{
    if (m_selectedEdge.IsNull()) {
        QMessageBox::warning(this, "Warning", "Please pick an edge first.");
        return;
    }
    emit signalFillet(m_selectedEdge, spinRadius->value());
}

void WidgetFillet::saveMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters = view->getSelectionFilters();
}

void WidgetFillet::restoreMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetFillet::onObjectSelected);
    view->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
    for (const auto& filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, filter.second);
    }
    m_isPicking = false;
}
