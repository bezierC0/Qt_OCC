#include "widget_chamfer.h"
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

WidgetChamfer::WidgetChamfer(QWidget* parent)
    : QWidget(parent), m_isPicking(false)
{
    setWindowTitle("Chamfer Feature");
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    auto* mainLayout = new QVBoxLayout(this);
    
    lblStatus = new QLabel("Ready to pick an edge.", this);
    mainLayout->addWidget(lblStatus);

    btnPick = new QPushButton("Pick Edge", this);
    mainLayout->addWidget(btnPick);

    auto* formLayout = new QFormLayout();
    spinDistance = new QDoubleSpinBox(this);
    spinDistance->setRange(0.01, 1000.0);
    spinDistance->setValue(1.0);
    
    formLayout->addRow("Distance:", spinDistance);
    mainLayout->addLayout(formLayout);

    btnApply = new QPushButton("Apply Chamfer", this);
    mainLayout->addWidget(btnApply);

    connect(btnPick, &QPushButton::clicked, this, &WidgetChamfer::onPickClicked);
    connect(btnApply, &QPushButton::clicked, this, &WidgetChamfer::onApplyClicked);
}

WidgetChamfer::~WidgetChamfer()
{
}

void WidgetChamfer::show()
{
    QWidget::show();
}

void WidgetChamfer::hide()
{
    if (m_isPicking) {
        restoreMouseState();
    }
    QWidget::hide();
}

void WidgetChamfer::closeEvent(QCloseEvent* event)
{
    if (m_isPicking) {
        restoreMouseState();
    }
    QWidget::closeEvent(event);
}

void WidgetChamfer::onPickClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    if (m_isPicking) return;

    saveMouseState();
    m_isPicking = true;

    view->clearSelectedObjects();
    view->updateSelectionFilter(TopAbs_EDGE, true);
    view->setMouseMode(View::MouseMode::SELECTION);

    connect(view, &OCCView::signalSpaceSelected, this, &WidgetChamfer::onObjectSelected);
    lblStatus->setText("Please select an edge.");
}

void WidgetChamfer::onObjectSelected(const TopoDS_Shape& shape)
{
    if (!m_isPicking || shape.IsNull()) return;
    if (shape.ShapeType() != TopAbs_EDGE) return;

    m_selectedEdge = shape;
    lblStatus->setText("Edge picked. Ready to chamfer.");
    restoreMouseState(); // Stop picking after one edge
}

void WidgetChamfer::onApplyClicked()
{
    if (m_selectedEdge.IsNull()) {
        QMessageBox::warning(this, "Warning", "Please pick an edge first.");
        return;
    }
    emit signalChamfer(m_selectedEdge, spinDistance->value());
}

void WidgetChamfer::saveMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters = view->getSelectionFilters();
}

void WidgetChamfer::restoreMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetChamfer::onObjectSelected);
    view->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
    for (const auto& filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, filter.second);
    }
    m_isPicking = false;
}
