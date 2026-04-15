#include "widget_hole.h"
#include "ui_widget_hole.h"

#include "OCCView.h"
#include "SelectedEntity.h"
#include "ViewManager.h"

#include <BRep_Tool.hxx>

#include <QCloseEvent>
#include <QMessageBox>

WidgetHole::WidgetHole(QWidget* parent)
    : QWidget(parent),
      ui(new Ui::WidgetHole),
      m_pickingState(Idle)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    connect(ui->pushButtonPickFace, &QPushButton::clicked, this, &WidgetHole::onPickFaceClicked);
    connect(ui->pushButtonPickPoint, &QPushButton::clicked, this, &WidgetHole::onPickPointClicked);
    connect(ui->pushButtonApply, &QPushButton::clicked, this, &WidgetHole::onApplyClicked);
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &WidgetHole::onCloseClicked);
}

WidgetHole::~WidgetHole()
{
    delete ui;
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
    ui->labelStatus->setText("Please select a Face.");
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
    ui->labelStatus->setText("Please select a Vertex for Hole center.");
}

void WidgetHole::onObjectSelected(const TopoDS_Shape& shape)
{
    if (m_pickingState == Idle || shape.IsNull()) return;

    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    if (m_pickingState == PickFace && shape.ShapeType() == TopAbs_FACE) {
        // 获取父形状
        auto selectedList = view->getSelectedObjects();
        if (!selectedList.empty()) {
            auto parentObj = selectedList.back()->GetParentInteractiveObject();
            auto aisShape = Handle(AIS_Shape)::DownCast(parentObj);
            if (aisShape) {
                m_parentShape = aisShape->Shape();
            }
        }

        m_selectedFace = shape;
        ui->labelStatus->setText("Face picked. Now pick a center vertex.");
        restoreMouseState();
    }
    else if (m_pickingState == PickPoint && shape.ShapeType() == TopAbs_VERTEX) {
        m_selectedPoint = shape;
        ui->labelStatus->setText("Point picked. Ready to apply.");
        restoreMouseState();
    }
}

void WidgetHole::onApplyClicked()
{
    if (m_parentShape.IsNull() || m_selectedFace.IsNull() || m_selectedPoint.IsNull()) {
        QMessageBox::warning(this, "Warning", "Please pick both a Face and a Vertex.");
        return;
    }

    // 根据当前 Tab 确定 hole 类型和参数
    const int tabIndex = ui->tabWidget->currentIndex();
    double radius = 5.0;
    double depth = 0.0;
    HoleType type = HoleType::ThruAll;

    switch (tabIndex) {
    case 0: // Through All
        type = HoleType::ThruAll;
        radius = ui->spinRadiusThruAll->value();
        break;
    case 1: // Thru Next
        type = HoleType::ThruNext;
        radius = ui->spinRadiusThruNext->value();
        break;
    case 2: // Until End
        type = HoleType::UntilEnd;
        radius = ui->spinRadiusUntilEnd->value();
        break;
    case 3: // Blind
        type = HoleType::Blind;
        radius = ui->spinRadiusBlind->value();
        depth = ui->spinDepthBlind->value();
        break;
    default:
        break;
    }

    emit signalHole(m_parentShape, m_selectedFace, m_selectedPoint, radius, static_cast<int>(type), depth);
}

void WidgetHole::onCloseClicked()
{
    close();
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
