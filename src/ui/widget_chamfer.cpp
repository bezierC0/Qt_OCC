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
#include "SelectionPickSession.h"
#include <BRep_Tool.hxx>

WidgetChamfer::WidgetChamfer(QWidget* parent)
    : QWidget(parent), m_pickSession(new SelectionPickSession(this))
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
    connect(m_pickSession, &SelectionPickSession::shapePicked,
            this, &WidgetChamfer::onObjectSelected);
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
    if (m_pickSession->isActive()) {
        restoreMouseState();
    }
    QWidget::hide();
}

void WidgetChamfer::closeEvent(QCloseEvent* event)
{
    if (m_pickSession->isActive()) {
        restoreMouseState();
    }
    QWidget::closeEvent(event);
}

void WidgetChamfer::onPickClicked()
{
    if (m_pickSession->isActive()) return;
    if (!m_pickSession->start({{TopAbs_EDGE}})) return;
    lblStatus->setText("Please select an edge.");
}

void WidgetChamfer::onObjectSelected(const TopoDS_Shape& shape)
{
    if (!m_pickSession->isActive() || shape.IsNull()) return;
    if (shape.ShapeType() != TopAbs_EDGE) return;

    m_selectedEdge = shape;
    lblStatus->setText("Edge picked. Ready to chamfer.");
    restoreMouseState(); // Stop picking after one edge
}

void WidgetChamfer::onApplyClicked()
{
    if (m_selectedEdge.IsNull()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please pick an edge first."));
        return;
    }
    emit signalChamfer(m_selectedEdge, spinDistance->value());
}

void WidgetChamfer::restoreMouseState()
{
    m_pickSession->stop();
}
