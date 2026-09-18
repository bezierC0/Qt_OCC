#include "DialogCreateRevol.h"

#include "OCCView.h"
#include "SelectedEntity.h"
#include "SelectionPickSession.h"
#include "ViewManager.h"

#include <QCloseEvent>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

DialogCreateRevol::DialogCreateRevol(QWidget* parent)
    : QDialog(parent), m_pickSession(new SelectionPickSession(this))
{
    setWindowTitle(tr("Create Revolved Solid"));
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    auto* layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    m_status = new QLabel(tr("Select a face to revolve."), this);
    layout->addWidget(m_status);

    auto* pickButton = new QPushButton(tr("Pick Face"), this);
    layout->addWidget(pickButton);
    connect(pickButton, &QPushButton::clicked, this, &DialogCreateRevol::onPickClicked);

    auto* form = new QFormLayout();
    m_x = new QDoubleSpinBox(this);
    m_y = new QDoubleSpinBox(this);
    m_z = new QDoubleSpinBox(this);
    for (auto* spin : {m_x, m_y, m_z}) {
        spin->setRange(-10000.0, 10000.0);
        spin->setDecimals(3);
    }
    form->addRow(tr("Axis point X:"), m_x);
    form->addRow(tr("Axis point Y:"), m_y);
    form->addRow(tr("Axis point Z:"), m_z);

    m_nx = new QDoubleSpinBox(this);
    m_ny = new QDoubleSpinBox(this);
    m_nz = new QDoubleSpinBox(this);
    for (auto* spin : {m_nx, m_ny, m_nz}) {
        spin->setRange(-10000.0, 10000.0);
        spin->setDecimals(3);
        spin->setSingleStep(0.1);
    }
    m_nz->setValue(1.0);
    form->addRow(tr("Axis direction X:"), m_nx);
    form->addRow(tr("Axis direction Y:"), m_ny);
    form->addRow(tr("Axis direction Z:"), m_nz);

    m_angle = new QDoubleSpinBox(this);
    m_angle->setRange(0.001, 360.0);
    m_angle->setDecimals(3);
    m_angle->setValue(360.0);
    form->addRow(tr("Angle (degrees):"), m_angle);
    layout->addLayout(form);

    auto* colorLayout = new QHBoxLayout();
    m_colorButton = new QPushButton(tr("Select Color"), this);
    colorLayout->addWidget(new QLabel(tr("Color:"), this));
    colorLayout->addWidget(m_colorButton);
    layout->addLayout(colorLayout);
    connect(m_colorButton, &QPushButton::clicked, this, &DialogCreateRevol::onColorClicked);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Create"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &DialogCreateRevol::onCreateClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_pickSession, &SelectionPickSession::shapePicked,
            this, &DialogCreateRevol::onShapePicked);
}

DialogCreateRevol::~DialogCreateRevol()
{
    m_pickSession->stop();
}

void DialogCreateRevol::show()
{
    QDialog::show();
    auto* view = ViewManager::getInstance().getActiveView();
    if (view) {
        const auto& selected = view->getSelectedObjects();
        if (selected.size() == 1 && selected.front()
            && !selected.front()->GetSelectedShape().IsNull()) {
            onShapePicked(selected.front()->GetSelectedShape()->Shape());
            if (!m_face.IsNull()) return;
        }
    }
    if (m_face.IsNull()) onPickClicked();
}

void DialogCreateRevol::done(int result)
{
    m_pickSession->stop();
    QDialog::done(result);
}

void DialogCreateRevol::closeEvent(QCloseEvent* event)
{
    m_pickSession->stop();
    QDialog::closeEvent(event);
}

void DialogCreateRevol::onPickClicked()
{
    m_face.Nullify();
    if (m_pickSession->start({{TopAbs_FACE}}))
        m_status->setText(tr("Select a face to revolve."));
    else
        m_status->setText(tr("No active view is available."));
}

void DialogCreateRevol::onShapePicked(const TopoDS_Shape& shape)
{
    if (shape.IsNull() || shape.ShapeType() != TopAbs_FACE) {
        m_status->setText(tr("Select a face to revolve."));
        return;
    }
    m_face = shape;
    m_pickSession->stop();
    m_status->setText(tr("Face selected."));
}

void DialogCreateRevol::onCreateClicked()
{
    if (m_face.IsNull()) {
        QMessageBox::warning(this, tr("Invalid Parameters"), tr("Select a face first."));
        return;
    }
    if (m_nx->value() == 0.0 && m_ny->value() == 0.0 && m_nz->value() == 0.0) {
        QMessageBox::warning(this, tr("Invalid Parameters"), tr("Axis direction must be nonzero."));
        return;
    }
    m_pickSession->stop();
    emit signalCreateRevol(m_face, m_x->value(), m_y->value(), m_z->value(),
                           m_nx->value(), m_ny->value(), m_nz->value(), m_angle->value(), m_color);
}

void DialogCreateRevol::onColorClicked()
{
    const QColor picked = QColorDialog::getColor(m_color, this, tr("Select Color"));
    if (picked.isValid()) {
        m_color = picked;
        m_colorButton->setStyleSheet(QString("background-color: %1").arg(picked.name()));
    }
}
