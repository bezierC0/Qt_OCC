#include "DialogCreateOffsetCurve.h"

#include "SelectionPickSession.h"
#include "ViewManager.h"
#include "OCCView.h"
#include "SelectedEntity.h"
#include "command/CommandCommon.h"
#include "command/ShapeCommandRegistry.h"

#include <TopExp_Explorer.hxx>
#include <Quantity_Color.hxx>

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

DialogCreateOffsetCurve::DialogCreateOffsetCurve(QWidget* parent)
    : QDialog(parent)
    , m_pickSession(new SelectionPickSession(this))
{
    setWindowTitle(tr("Create Offset Curve"));
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    auto* layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    m_status = new QLabel(tr("Select one edge or a single-edge wire."), this);
    layout->addWidget(m_status);

    auto* pickButton = new QPushButton(tr("Pick Curve"), this);
    layout->addWidget(pickButton);
    connect(pickButton, &QPushButton::clicked, this, &DialogCreateOffsetCurve::onPickClicked);

    auto* form = new QFormLayout();
    m_distance = new QDoubleSpinBox(this);
    m_distance->setRange(-10000.0, 10000.0);
    m_distance->setDecimals(3);
    m_distance->setValue(10.0);
    form->addRow(tr("Signed distance:"), m_distance);

    m_normalX = new QDoubleSpinBox(this);
    m_normalY = new QDoubleSpinBox(this);
    m_normalZ = new QDoubleSpinBox(this);
    for (auto* spin : {m_normalX, m_normalY, m_normalZ}) {
        spin->setRange(-1.0, 1.0);
        spin->setSingleStep(0.1);
    }
    m_normalZ->setValue(1.0);
    form->addRow(tr("Reference X:"), m_normalX);
    form->addRow(tr("Reference Y:"), m_normalY);
    form->addRow(tr("Reference Z:"), m_normalZ);
    layout->addLayout(form);

    auto* colorLayout = new QHBoxLayout();
    m_colorButton = new QPushButton(tr("Select Color"), this);
    colorLayout->addWidget(new QLabel(tr("Color:"), this));
    colorLayout->addWidget(m_colorButton);
    layout->addLayout(colorLayout);
    connect(m_colorButton, &QPushButton::clicked, this, &DialogCreateOffsetCurve::onColorClicked);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Create"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &DialogCreateOffsetCurve::onCreateClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    for (auto* spin : {m_distance, m_normalX, m_normalY, m_normalZ}) {
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &DialogCreateOffsetCurve::updatePreview);
    }
    connect(m_pickSession, &SelectionPickSession::shapePicked,
            this, &DialogCreateOffsetCurve::onShapePicked);
}

DialogCreateOffsetCurve::~DialogCreateOffsetCurve()
{
    m_pickSession->stop();
    clearPreview();
}

void DialogCreateOffsetCurve::show()
{
    QDialog::show();
    auto* view = ViewManager::getInstance().getActiveView();
    if (view) {
        const auto& selected = view->getSelectedObjects();
        if (selected.size() == 1 && selected.front()
            && !selected.front()->GetSelectedShape().IsNull()) {
            onShapePicked(selected.front()->GetSelectedShape()->Shape());
            return;
        }
    }
    if (m_basis.IsNull()) onPickClicked();
}

void DialogCreateOffsetCurve::done(int result)
{
    m_pickSession->stop();
    clearPreview();
    QDialog::done(result);
}

void DialogCreateOffsetCurve::closeEvent(QCloseEvent* event)
{
    m_pickSession->stop();
    clearPreview();
    QDialog::closeEvent(event);
}

void DialogCreateOffsetCurve::onPickClicked()
{
    clearPreview();
    m_basis.Nullify();
    if (m_pickSession->start({{TopAbs_EDGE, TopAbs_WIRE}}))
        m_status->setText(tr("Select one edge or a single-edge wire."));
    else
        m_status->setText(tr("No active view is available."));
}

void DialogCreateOffsetCurve::onShapePicked(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) return;
    if (shape.ShapeType() != TopAbs_EDGE && shape.ShapeType() != TopAbs_WIRE) {
        m_status->setText(tr("Select one edge or a single-edge wire."));
        return;
    }
    TopExp_Explorer edges(shape, TopAbs_EDGE);
    if (!edges.More()) return;
    edges.Next();
    if (edges.More()) {
        m_status->setText(tr("This wire has multiple edges. Select one edge."));
        return;
    }
    m_basis = shape;
    m_pickSession->stop();
    m_status->setText(tr("Curve selected."));
    updatePreview();
}

void DialogCreateOffsetCurve::updatePreview()
{
    clearPreview();
    if (m_basis.IsNull()) return;

    CoreApi::ShapeParams p;
    p[CoreApi::Param::BASIS] = QVariant::fromValue(m_basis);
    p[CoreApi::Param::DISTANCE] = m_distance->value();
    p[CoreApi::Param::NX] = m_normalX->value();
    p[CoreApi::Param::NY] = m_normalY->value();
    p[CoreApi::Param::NZ] = m_normalZ->value();
    const TopoDS_Shape preview = CoreApi::ShapeCommandRegistry::instance().execute("CreateOffsetCurve", p);
    if (preview.IsNull()) {
        m_status->setText(tr("Cannot offset this curve with the current values."));
        return;
    }

    m_previewView = ViewManager::getInstance().getActiveView();
    if (!m_previewView || m_previewView->Context().IsNull()) return;
    m_previewShape = new AIS_Shape(preview);
    m_previewShape->SetColor(Quantity_Color(1.0, 1.0, 0.3, Quantity_TOC_RGB));
    m_previewShape->SetTransparency(0.3);
    m_previewView->Context()->Display(m_previewShape, false);
    m_previewView->Context()->UpdateCurrentViewer();
    m_status->setText(tr("Curve selected."));
}

void DialogCreateOffsetCurve::clearPreview()
{
    if (m_previewView && !m_previewShape.IsNull() && !m_previewView->Context().IsNull()) {
        m_previewView->Context()->Remove(m_previewShape, false);
        m_previewView->Context()->UpdateCurrentViewer();
    }
    m_previewShape.Nullify();
    m_previewView.clear();
}

void DialogCreateOffsetCurve::onCreateClicked()
{
    if (m_basis.IsNull()) {
        QMessageBox::warning(this, tr("Invalid Parameters"), tr("Select a curve first."));
        return;
    }
    if (m_distance->value() == 0.0
        || (m_normalX->value() == 0.0 && m_normalY->value() == 0.0 && m_normalZ->value() == 0.0)) {
        QMessageBox::warning(this, tr("Invalid Parameters"),
                             tr("Distance and reference direction must be nonzero."));
        return;
    }
    m_pickSession->stop();
    clearPreview();
    emit signalCreateOffsetCurve(m_basis, m_distance->value(),
                                  m_normalX->value(), m_normalY->value(), m_normalZ->value(), m_color);
}

void DialogCreateOffsetCurve::onColorClicked()
{
    const QColor picked = QColorDialog::getColor(m_color, this, tr("Select Color"));
    if (picked.isValid()) {
        m_color = picked;
        m_colorButton->setStyleSheet(QString("background-color: %1").arg(picked.name()));
    }
}
