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
#include "SelectionPickSession.h"
#include <BRep_Tool.hxx>

WidgetFillet::WidgetFillet(QWidget* parent)
    : QWidget(parent), m_pickSession(new SelectionPickSession(this))
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
    connect(m_pickSession, &SelectionPickSession::shapePicked,
            this, &WidgetFillet::onObjectSelected);
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
    if (m_pickSession->isActive()) {
        restoreMouseState();
    }
    QWidget::hide();
}

void WidgetFillet::closeEvent(QCloseEvent* event)
{
    if (m_pickSession->isActive()) {
        restoreMouseState();
    }
    QWidget::closeEvent(event);
}

void WidgetFillet::onPickClicked()
{
    if (m_pickSession->isActive()) return;
    if (!m_pickSession->start({{TopAbs_EDGE}})) return;
    lblStatus->setText("Please select an edge.");
}

void WidgetFillet::onObjectSelected(const TopoDS_Shape& shape)
{
    if (!m_pickSession->isActive() || shape.IsNull()) return;
    if (shape.ShapeType() != TopAbs_EDGE) return;

    m_selectedEdge = shape;
    lblStatus->setText("Edge picked. Ready to fillet.");
    restoreMouseState(); // Stop picking after one edge
}

void WidgetFillet::onApplyClicked()
{
    if (m_selectedEdge.IsNull()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please pick an edge first."));
        return;
    }
    emit signalFillet(m_selectedEdge, spinRadius->value());
}

void WidgetFillet::restoreMouseState()
{
    m_pickSession->stop();
}
