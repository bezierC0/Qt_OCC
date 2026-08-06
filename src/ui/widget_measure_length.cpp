#include "widget_measure_length.h"
#include "ui_widget_measure_length.h"
#include "ViewManager.h"
#include "SelectionPickSession.h"
#include "OCCView.h"
#include "SelectedEntity.h"

#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

#include <QMessageBox>

WidgetMeasureLength::WidgetMeasureLength(QWidget *parent)
    : QWidget(parent), ui(new Ui::WidgetMeasureLength),
      m_pickSession(new SelectionPickSession(this)), m_totalLength(0.0)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    connect(ui->pushButtonPick, &QPushButton::clicked, this, &WidgetMeasureLength::onPickClicked);
    connect(ui->pushButtonClear, &QPushButton::clicked, this, &WidgetMeasureLength::onClearClicked);
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &WidgetMeasureLength::onCloseClicked);
    connect(m_pickSession, &SelectionPickSession::shapePicked,
            this, &WidgetMeasureLength::onObjectSelected);
}

WidgetMeasureLength::~WidgetMeasureLength()
{
    delete ui;
}

void WidgetMeasureLength::show()
{
    QWidget::show();

    onClearClicked();

    auto view = ViewManager::getInstance().getActiveView();
    if (view) {
        const auto &selectedList = view->getSelectedObjects();
        if (!selectedList.empty()) {
            bool hasValid = false;
            for (const auto &ent : selectedList) {
                if (!ent)
                    continue;
                auto aisShape = ent->GetSelectedShape();
                if (!aisShape.IsNull()) {
                    TopoDS_Shape shape = aisShape->Shape();
                    if (shape.ShapeType() == TopAbs_EDGE || shape.ShapeType() == TopAbs_WIRE) {
                        calculateAndAddLength(shape);
                        hasValid = true;
                    }
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

void WidgetMeasureLength::hide()
{
    if (m_pickSession->isActive()) {
        restoreMouseState();
    }
    QWidget::hide();
}

void WidgetMeasureLength::closeEvent(QCloseEvent *event)
{
    if (m_pickSession->isActive()) {
        restoreMouseState();
    }
    QWidget::closeEvent(event);
}

void WidgetMeasureLength::onPickClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view)
        return;

    if (m_pickSession->isActive()) {
        return;
    }

    if (!m_pickSession->start({{TopAbs_EDGE, TopAbs_WIRE}})) return;

    ui->labelStatus->setText("Please select edges or wires.");
}

void WidgetMeasureLength::onClearClicked()
{
    m_selectedShapes.clear();
    m_totalLength = 0.0;
    updateUI();

    auto view = ViewManager::getInstance().getActiveView();
    if (view && m_pickSession->isActive()) {
        view->clearSelectedObjects();
    }
}

void WidgetMeasureLength::onObjectSelected(const TopoDS_Shape &shape)
{
    if (!m_pickSession->isActive() || shape.IsNull())
        return;

    if (shape.ShapeType() != TopAbs_EDGE && shape.ShapeType() != TopAbs_WIRE) {
        return;
    }

    // Check if already selected
    for (const auto &s : m_selectedShapes) {
        if (s.IsSame(shape)) {
            return; // Duplicate
        }
    }

    calculateAndAddLength(shape);
    updateUI();
}

void WidgetMeasureLength::calculateAndAddLength(const TopoDS_Shape &shape)
{
    GProp_GProps props;
    BRepGProp::LinearProperties(shape, props);
    double length = props.Mass();

    m_selectedShapes.push_back(shape);
    m_totalLength += length;
}

void WidgetMeasureLength::updateUI()
{
    ui->labelSelectedCountValue->setText(QString::number(m_selectedShapes.size()));
    ui->labelTotalLengthValue->setText(QString::number(m_totalLength, 'f', 4));
}

void WidgetMeasureLength::onCloseClicked()
{
    close();
}

void WidgetMeasureLength::restoreMouseState()
{
    m_pickSession->stop();
    ui->labelStatus->setText("Measurement complete.");
}
