#include "widget_hole.h"
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

WidgetHole::WidgetHole(QWidget* parent)
    : QWidget(parent), m_pickingState(Idle)
{
    setWindowTitle("Hole Feature");
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    auto* mainLayout = new QVBoxLayout(this);
    
    lblStatus = new QLabel("Ready.", this);
    mainLayout->addWidget(lblStatus);

    btnPickFace = new QPushButton("Pick Target Face", this);
    mainLayout->addWidget(btnPickFace);

    btnPickPoint = new QPushButton("Pick Center Vertex", this);
    mainLayout->addWidget(btnPickPoint);

    auto* formLayout = new QFormLayout();
    spinRadius = new QDoubleSpinBox(this);
    spinRadius->setRange(0.01, 1000.0);
    spinRadius->setValue(5.0);
    
    formLayout->addRow("Radius:", spinRadius);
    mainLayout->addLayout(formLayout);

    btnApply = new QPushButton("Apply Hole", this);
    mainLayout->addWidget(btnApply);

    connect(btnPickFace, &QPushButton::clicked, this, &WidgetHole::onPickFaceClicked);
    connect(btnPickPoint, &QPushButton::clicked, this, &WidgetHole::onPickPointClicked);
    connect(btnApply, &QPushButton::clicked, this, &WidgetHole::onApplyClicked);
}

WidgetHole::~WidgetHole()
{
}

void WidgetHole::show()
{
    QWidget::show();
}

void WidgetHole::hide()
{
    if (m_pickingState != Idle) {
        restoreMouseState();
    }
    QWidget::hide();
}

void WidgetHole::closeEvent(QCloseEvent* event)
{
    if (m_pickingState != Idle) {
        restoreMouseState();
    }
    QWidget::closeEvent(event);
}

void WidgetHole::onPickFaceClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    
    if (m_pickingState != Idle) {
        restoreMouseState();
    }

    saveMouseState();
    m_pickingState = PickFace;

    view->clearSelectedObjects();
    view->updateSelectionFilter(TopAbs_FACE, true);
    view->setMouseMode(View::MouseMode::SELECTION);

    connect(view, &OCCView::signalSpaceSelected, this, &WidgetHole::onObjectSelected, Qt::UniqueConnection);
    lblStatus->setText("Please select a Face.");
}

void WidgetHole::onPickPointClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    if (m_parentShape.IsNull() || m_selectedFace.IsNull()) {
        QMessageBox::warning(this, "Warning", "Please pick a target face first.");
        return;
    }
    
    if (m_pickingState != Idle) {
        restoreMouseState();
    }

    saveMouseState();
    m_pickingState = PickPoint;

    view->clearSelectedObjects();
    view->updateSelectionFilter(TopAbs_VERTEX, true);
    view->setMouseMode(View::MouseMode::SELECTION);

    connect(view, &OCCView::signalSpaceSelected, this, &WidgetHole::onObjectSelected, Qt::UniqueConnection);
    lblStatus->setText("Please select a Vertex for Hole center.");
}

void WidgetHole::onObjectSelected(const TopoDS_Shape& shape)
{
    if (m_pickingState == Idle || shape.IsNull()) return;

    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    if (m_pickingState == PickFace && shape.ShapeType() == TopAbs_FACE) {
        // Capture parent shape
        auto selectedList = view->getSelectedObjects();
        if (!selectedList.empty()) {
            auto parentObj = selectedList.back()->GetParentInteractiveObject();
            auto aisShape = Handle(AIS_Shape)::DownCast(parentObj);
            if (aisShape) {
                m_parentShape = aisShape->Shape();
            }
        }
        
        m_selectedFace = shape;
        lblStatus->setText("Face picked. Now pick a center vertex.");
        restoreMouseState(); 
    }
    else if (m_pickingState == PickPoint && shape.ShapeType() == TopAbs_VERTEX) {
        m_selectedPoint = shape;
        lblStatus->setText("Point picked. Ready to apply.");
        restoreMouseState(); 
    }
}

void WidgetHole::onApplyClicked()
{
    if (m_parentShape.IsNull() || m_selectedFace.IsNull() || m_selectedPoint.IsNull()) {
        QMessageBox::warning(this, "Warning", "Please pick both a Face and a Vertex.");
        return;
    }
    emit signalHole(m_parentShape, m_selectedFace, m_selectedPoint, spinRadius->value());
}

void WidgetHole::saveMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters = view->getSelectionFilters();
}

void WidgetHole::restoreMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetHole::onObjectSelected);
    view->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
    for (const auto& filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, filter.second);
    }
    m_pickingState = Idle;
}
