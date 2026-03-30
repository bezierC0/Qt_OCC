#include "widget_measure_arc_length.h"
#include "ui_widget_measure_arc_length.h"

#include "OCCView.h"
#include "SelectedEntity.h"
#include "ViewManager.h"

#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

WidgetMeasureArcLength::WidgetMeasureArcLength(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::WidgetMeasureArcLength),
      m_isPicking(false),
      m_totalArcLength(0.0)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    connect(ui->pushButtonPick, &QPushButton::clicked, this, &WidgetMeasureArcLength::onPickClicked);
    connect(ui->pushButtonClear, &QPushButton::clicked, this, &WidgetMeasureArcLength::onClearClicked);
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &WidgetMeasureArcLength::onCloseClicked);
}

WidgetMeasureArcLength::~WidgetMeasureArcLength()
{
    delete ui;
}

void WidgetMeasureArcLength::show()
{
    QWidget::show();

    onClearClicked();

    auto view = ViewManager::getInstance().getActiveView();
    if (view) {
        const auto &selectedList = view->getSelectedObjects();
        if (!selectedList.empty()) {
            bool hasValid = false;
            for (const auto &ent : selectedList) {
                if (!ent) {
                    continue;
                }

                auto aisShape = ent->GetSelectedShape();
                if (aisShape.IsNull()) {
                    continue;
                }

                const TopoDS_Shape shape = aisShape->Shape();
                if (isArcEdge(shape)) {
                    calculateAndAddArcLength(shape);
                    hasValid = true;
                }
            }

            if (hasValid) {
                updateUI();
                return;
            }
        }
    }

    onPickClicked();
}

void WidgetMeasureArcLength::hide()
{
    if (m_isPicking) {
        restoreMouseState();
    }
    QWidget::hide();
}

void WidgetMeasureArcLength::closeEvent(QCloseEvent *event)
{
    if (m_isPicking) {
        restoreMouseState();
    }
    QWidget::closeEvent(event);
}

void WidgetMeasureArcLength::onPickClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }

    if (m_isPicking) {
        return;
    }

    saveMouseState();
    m_isPicking = true;

    view->clearSelectedObjects();
    view->updateSelectionFilter(TopAbs_EDGE, true);
    view->setMouseMode(View::MouseMode::SELECTION);

    connect(view, &OCCView::signalSpaceSelected, this, &WidgetMeasureArcLength::onObjectSelected);

    ui->labelStatus->setText("Please select arc edges.");
}

void WidgetMeasureArcLength::onClearClicked()
{
    m_selectedShapes.clear();
    m_totalArcLength = 0.0;
    updateUI();

    auto view = ViewManager::getInstance().getActiveView();
    if (view && m_isPicking) {
        view->clearSelectedObjects();
    }
}

void WidgetMeasureArcLength::onObjectSelected(const TopoDS_Shape &shape)
{
    if (!m_isPicking || shape.IsNull() || !isArcEdge(shape)) {
        return;
    }

    for (const auto &selectedShape : m_selectedShapes) {
        if (selectedShape.IsSame(shape)) {
            return;
        }
    }

    calculateAndAddArcLength(shape);
    updateUI();
}

void WidgetMeasureArcLength::onCloseClicked()
{
    close();
}

void WidgetMeasureArcLength::saveMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }

    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters = view->getSelectionFilters();
}

void WidgetMeasureArcLength::restoreMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }

    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetMeasureArcLength::onObjectSelected);

    view->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
    for (const auto &filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, filter.second);
    }

    m_isPicking = false;
    ui->labelStatus->setText("Measurement complete.");
}

void WidgetMeasureArcLength::updateUI()
{
    ui->labelSelectedCountValue->setText(QString::number(m_selectedShapes.size()));
    ui->labelTotalArcLengthValue->setText(QString::number(m_totalArcLength, 'f', 4));
}

bool WidgetMeasureArcLength::isArcEdge(const TopoDS_Shape &shape) const
{
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) {
        return false;
    }

    const TopoDS_Edge edge = TopoDS::Edge(shape);
    Standard_Real first = 0.0;
    Standard_Real last = 0.0;
    const Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
    if (curve.IsNull()) {
        return false;
    }

    const GeomAdaptor_Curve adaptor(curve, first, last);
    const GeomAbs_CurveType curveType = adaptor.GetType();
    return curveType == GeomAbs_Circle || curveType == GeomAbs_Ellipse;
}

void WidgetMeasureArcLength::calculateAndAddArcLength(const TopoDS_Shape &shape)
{
    GProp_GProps props;
    BRepGProp::LinearProperties(shape, props);

    m_selectedShapes.push_back(shape);
    m_totalArcLength += props.Mass();
}
