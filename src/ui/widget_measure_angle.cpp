#include "widget_measure_angle.h"
#include "ui_widget_measure_angle.h"

#include "OCCView.h"
#include "SelectedEntity.h"
#include "ViewManager.h"

#include <BRep_Tool.hxx>
#include <Geom_Line.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

namespace
{
constexpr double kRadToDeg = 57.29577951308232;
}

WidgetMeasureAngle::WidgetMeasureAngle(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::WidgetMeasureAngle),
      m_isPicking(false),
      m_pickMode(PickMode::Unknown),
      m_angleDeg(0.0),
      m_angleRad(0.0),
      m_hasMeasurement(false)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    connect(ui->pushButtonPick, &QPushButton::clicked, this, &WidgetMeasureAngle::onPickClicked);
    connect(ui->pushButtonClear, &QPushButton::clicked, this, &WidgetMeasureAngle::onClearClicked);
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &WidgetMeasureAngle::onCloseClicked);
}

WidgetMeasureAngle::~WidgetMeasureAngle()
{
    delete ui;
}

void WidgetMeasureAngle::show()
{
    QWidget::show();

    resetMeasurement();
    if (tryLoadFromCurrentSelection()) {
        updateUI();
        return;
    }

    onPickClicked();
}

void WidgetMeasureAngle::hide()
{
    if (m_isPicking) {
        restoreMouseState();
    }
    QWidget::hide();
}

void WidgetMeasureAngle::closeEvent(QCloseEvent *event)
{
    if (m_isPicking) {
        restoreMouseState();
    }
    QWidget::closeEvent(event);
}

void WidgetMeasureAngle::onPickClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }

    if (m_isPicking) {
        restoreMouseState();
    }

    saveMouseState();
    resetMeasurement();
    m_isPicking = true;

    view->clearSelectedObjects();
    for (const auto &filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, false);
    }
    view->updateSelectionFilter(TopAbs_EDGE, true);
    view->updateSelectionFilter(TopAbs_VERTEX, true);
    view->setMouseMode(View::MouseMode::SELECTION);

    connect(view, &OCCView::signalSpaceSelected, this, &WidgetMeasureAngle::onObjectSelected);

    updateUI();
}

void WidgetMeasureAngle::onClearClicked()
{
    resetMeasurement();
    updateUI();

    auto view = ViewManager::getInstance().getActiveView();
    if (view && m_isPicking) {
        view->clearSelectedObjects();
    }
}

void WidgetMeasureAngle::onCloseClicked()
{
    close();
}

void WidgetMeasureAngle::onObjectSelected(const TopoDS_Shape &shape)
{
    if (!m_isPicking || shape.IsNull()) {
        return;
    }

    if (m_pickMode == PickMode::Unknown) {
        if (isLineEdge(shape)) {
            m_pickMode = PickMode::EdgeAngle;
        } else if (isVertexShape(shape)) {
            m_pickMode = PickMode::VertexAngle;
        } else {
            ui->labelStatus->setText("Please select a line edge or a vertex.");
            return;
        }
    }

    if (!canAcceptShape(shape)) {
        ui->labelStatus->setText(
            m_pickMode == PickMode::EdgeAngle ? "Please continue selecting line edges."
                                              : "Please continue selecting vertices.");
        return;
    }

    for (const auto &selectedShape : m_selectedShapes) {
        if (selectedShape.IsSame(shape)) {
            return;
        }
    }

    m_selectedShapes.push_back(shape);
    if (calculateAngle()) {
        restoreMouseState();
    }

    updateUI();
}

void WidgetMeasureAngle::saveMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }

    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters = view->getSelectionFilters();
}

void WidgetMeasureAngle::restoreMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }

    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetMeasureAngle::onObjectSelected);
    view->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
    for (const auto &filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, filter.second);
    }

    m_isPicking = false;
}

void WidgetMeasureAngle::resetMeasurement()
{
    m_pickMode = PickMode::Unknown;
    m_selectedShapes.clear();
    m_angleDeg = 0.0;
    m_angleRad = 0.0;
    m_hasMeasurement = false;
}

void WidgetMeasureAngle::updateUI()
{
    QString modeText = "Auto";
    if (m_pickMode == PickMode::EdgeAngle) {
        modeText = "Two Lines";
    } else if (m_pickMode == PickMode::VertexAngle) {
        modeText = "Three Points";
    }

    ui->labelModeValue->setText(modeText);
    ui->labelSelectedCountValue->setText(QString::number(m_selectedShapes.size()));
    ui->labelAngleDegValue->setText(QString::number(m_angleDeg, 'f', 4));
    ui->labelAngleRadValue->setText(QString::number(m_angleRad, 'f', 6));

    if (m_isPicking) {
        if (m_pickMode == PickMode::Unknown) {
            ui->labelStatus->setText("Select two line edges or three vertices.");
        } else {
            const int remaining = requiredSelectionCount() - static_cast<int>(m_selectedShapes.size());
            if (remaining > 0) {
                ui->labelStatus->setText(
                    m_pickMode == PickMode::EdgeAngle
                        ? QString("Please select %1 more line edge(s).").arg(remaining)
                        : QString("Please select %1 more vertex/vertices.").arg(remaining));
            } else if (m_hasMeasurement) {
                ui->labelStatus->setText("Measurement complete.");
            } else {
                ui->labelStatus->setText("Selected items cannot form a valid angle. Click 'Clear' or 'Pick' and try again.");
            }
        }
    } else if (m_hasMeasurement) {
        ui->labelStatus->setText("Measurement complete.");
    } else if (!m_selectedShapes.empty()) {
        ui->labelStatus->setText("Selected items cannot form a valid angle.");
    } else {
        ui->labelStatus->setText("Click 'Pick' and select two lines or three points.");
    }
}

bool WidgetMeasureAngle::isLineEdge(const TopoDS_Shape &shape) const
{
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) {
        return false;
    }

    Standard_Real first = 0.0;
    Standard_Real last = 0.0;
    const auto curve = BRep_Tool::Curve(TopoDS::Edge(shape), first, last);
    return !Handle(Geom_Line)::DownCast(curve).IsNull();
}

bool WidgetMeasureAngle::isVertexShape(const TopoDS_Shape &shape) const
{
    return !shape.IsNull() && shape.ShapeType() == TopAbs_VERTEX;
}

bool WidgetMeasureAngle::canAcceptShape(const TopoDS_Shape &shape) const
{
    if (m_pickMode == PickMode::EdgeAngle) {
        return isLineEdge(shape);
    }
    if (m_pickMode == PickMode::VertexAngle) {
        return isVertexShape(shape);
    }
    return isLineEdge(shape) || isVertexShape(shape);
}

int WidgetMeasureAngle::requiredSelectionCount() const
{
    if (m_pickMode == PickMode::EdgeAngle) {
        return 2;
    }
    if (m_pickMode == PickMode::VertexAngle) {
        return 3;
    }
    return 0;
}

bool WidgetMeasureAngle::tryLoadFromCurrentSelection()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return false;
    }

    const auto &selectedList = view->getSelectedObjects();
    if (selectedList.size() == 2) {
        std::vector<TopoDS_Shape> shapes;
        for (const auto &entity : selectedList) {
            if (!entity) {
                return false;
            }

            auto aisShape = entity->GetSelectedShape();
            if (aisShape.IsNull() || !isLineEdge(aisShape->Shape())) {
                return false;
            }

            shapes.push_back(aisShape->Shape());
        }

        m_pickMode = PickMode::EdgeAngle;
        m_selectedShapes = std::move(shapes);
        return calculateAngle();
    }

    if (selectedList.size() == 3) {
        std::vector<TopoDS_Shape> shapes;
        for (const auto &entity : selectedList) {
            if (!entity) {
                return false;
            }

            auto aisShape = entity->GetSelectedShape();
            if (aisShape.IsNull() || !isVertexShape(aisShape->Shape())) {
                return false;
            }

            shapes.push_back(aisShape->Shape());
        }

        m_pickMode = PickMode::VertexAngle;
        m_selectedShapes = std::move(shapes);
        return calculateAngle();
    }

    return false;
}

bool WidgetMeasureAngle::calculateAngle()
{
    if (m_pickMode == PickMode::EdgeAngle) {
        if (m_selectedShapes.size() != 2) {
            return false;
        }

        Standard_Real first1 = 0.0;
        Standard_Real last1 = 0.0;
        Standard_Real first2 = 0.0;
        Standard_Real last2 = 0.0;
        const auto curve1 = BRep_Tool::Curve(TopoDS::Edge(m_selectedShapes.at(0)), first1, last1);
        const auto curve2 = BRep_Tool::Curve(TopoDS::Edge(m_selectedShapes.at(1)), first2, last2);
        const auto line1 = Handle(Geom_Line)::DownCast(curve1);
        const auto line2 = Handle(Geom_Line)::DownCast(curve2);
        if (line1.IsNull() || line2.IsNull()) {
            return false;
        }

        m_angleRad = line1->Position().Direction().Angle(line2->Position().Direction());
        m_angleDeg = m_angleRad * kRadToDeg;
        m_hasMeasurement = true;
        return true;
    }

    if (m_pickMode == PickMode::VertexAngle) {
        if (m_selectedShapes.size() != 3) {
            return false;
        }

        const gp_Pnt p1 = BRep_Tool::Pnt(TopoDS::Vertex(m_selectedShapes.at(0)));
        const gp_Pnt p2 = BRep_Tool::Pnt(TopoDS::Vertex(m_selectedShapes.at(1)));
        const gp_Pnt p3 = BRep_Tool::Pnt(TopoDS::Vertex(m_selectedShapes.at(2)));

        const gp_Vec v1(p2, p1);
        const gp_Vec v2(p2, p3);
        if (v1.Magnitude() <= 1.0e-12 || v2.Magnitude() <= 1.0e-12) {
            return false;
        }

        m_angleRad = v1.Angle(v2);
        m_angleDeg = m_angleRad * kRadToDeg;
        m_hasMeasurement = true;
        return true;
    }

    return false;
}
