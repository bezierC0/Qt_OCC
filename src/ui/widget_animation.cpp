#include "widget_animation.h"
#include "ui_widget_animation.h"

#include "OCCView.h"
#include "SelectedEntity.h"
#include "ViewManager.h"

// OCC
#include <AIS_Shape.hxx>
#include <BRep_Tool.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Name.hxx>
#include <gp_Vec.hxx>
#include <gp_Trsf.hxx>

// Qt
#include <QMessageBox>
#include <QTimer>
#include <QListWidgetItem>
#include <QCloseEvent>

namespace
{
/// Returns a human-readable display name for an AIS interactive object.
QString getObjectDisplayName(const Handle(AIS_InteractiveObject) & obj)
{
    if (obj.IsNull()) {
        return QStringLiteral("(Unknown)");
    }

    Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(obj);
    if (!aisShape.IsNull()) {
        switch (aisShape->Shape().ShapeType()) {
        case TopAbs_SOLID:    return QStringLiteral("Solid");
        case TopAbs_SHELL:    return QStringLiteral("Shell");
        case TopAbs_COMPOUND: return QStringLiteral("Compound");
        default:              break;
        }
    }

    return QStringLiteral("Shape");
}

/// Builds the direction label string from a gp_Vec.
QString directionLabel(const gp_Vec &d)
{
    if (d.X() > 0.5)       return QStringLiteral("+X");
    if (d.X() < -0.5)      return QStringLiteral("-X");
    if (d.Y() > 0.5)       return QStringLiteral("+Y");
    if (d.Y() < -0.5)      return QStringLiteral("-Y");
    if (d.Z() > 0.5)       return QStringLiteral("+Z");
    return                         QStringLiteral("-Z");
}

/// Builds the display text for a single step entry in the list widget.
QString makeStepLabel(int index, const AnimationStep &step)
{
    return QString("[%1] %2  %3  %4 mm")
        .arg(index + 1)
        .arg(step.objectName)
        .arg(directionLabel(step.direction))
        .arg(step.distance, 0, 'f', 2);
}
} // namespace

WidgetAnimation::WidgetAnimation(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::WidgetAnimation),
      m_timer(new QTimer(this))
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    m_timer->setInterval(kTimerIntervalMs);

    connect(ui->pushButtonPick,   &QPushButton::clicked,  this, &WidgetAnimation::onPickClicked);
    connect(ui->pushButtonAdd,    &QPushButton::clicked,  this, &WidgetAnimation::onAddStepClicked);
    connect(ui->pushButtonRemove, &QPushButton::clicked,  this, &WidgetAnimation::onRemoveStepClicked);
    connect(ui->pushButtonPlay,   &QPushButton::clicked,  this, &WidgetAnimation::onPlayClicked);
    connect(ui->pushButtonPause,  &QPushButton::clicked,  this, &WidgetAnimation::onPauseClicked);
    connect(ui->pushButtonRewind, &QPushButton::clicked,  this, &WidgetAnimation::onRewindClicked);
    connect(ui->pushButtonClose,  &QPushButton::clicked,  this, &WidgetAnimation::onCloseClicked);
    connect(m_timer,              &QTimer::timeout,       this, &WidgetAnimation::onTimerTick);
    connect(ui->sliderSpeed,      &QSlider::valueChanged, this, &WidgetAnimation::onSpeedChanged);

    updateUI();
}

WidgetAnimation::~WidgetAnimation()
{
    delete ui;
}

void WidgetAnimation::show()
{
    QWidget::show();
    updateUI();
}

void WidgetAnimation::hide()
{
    stopAnimation();
    if (m_isPicking) {
        restoreMouseState();
    }
    QWidget::hide();
}

void WidgetAnimation::closeEvent(QCloseEvent *event)
{
    stopAnimation();
    if (m_isPicking) {
        restoreMouseState();
    }
    QWidget::closeEvent(event);
}

// Pick handling
void WidgetAnimation::onPickClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }

    if (m_isPicking) {
        restoreMouseState();
    }

    saveMouseState();
    m_isPicking = true;

    // Enable selection for solid-level shapes
    view->clearSelectedObjects();
    for (const auto &filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, false);
    }
    view->updateSelectionFilter(TopAbs_SOLID,    true);
    view->setMouseMode(View::MouseMode::SELECTION);

    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetAnimation::onObjectSelected);
    connect(view,    &OCCView::signalSpaceSelected, this, &WidgetAnimation::onObjectSelected);

    ui->labelStatus->setText(tr("Click a part to select it..."));
}

void WidgetAnimation::onObjectSelected(const TopoDS_Shape &shape)
{
    if (!m_isPicking) {
        return;
    }

    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }

    const auto selectedObjects = view->getSelectedObjects();
    if (selectedObjects.empty()) {
        return;
    }

    auto entity = selectedObjects.at(0);
    if (!entity) {
        return;
    }

    // Prefer the parent interactive object (e.g., the whole solid/assembly node)
    m_pickedObject = entity->GetParentInteractiveObject();
    if (m_pickedObject.IsNull()) {
        auto aisShape = entity->GetSelectedShape();
        if (!aisShape.IsNull()) {
            m_pickedObject = aisShape;
        }
    }

    m_pickedName = getObjectDisplayName(m_pickedObject);

    restoreMouseState();
    updateUI();
}

// Step management
void WidgetAnimation::onAddStepClicked()
{
    if (m_pickedObject.IsNull()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please pick a part first."));
        return;
    }

    AnimationStep step;
    step.object       = m_pickedObject;
    step.objectName   = m_pickedName;
    step.direction    = getSelectedDirection();
    step.distance     = ui->spinBoxDistance->value();
    step.originalTrsf = m_pickedObject->LocalTransformation();

    m_steps.push_back(step);

    const int idx = static_cast<int>(m_steps.size()) - 1;
    ui->listWidgetSteps->addItem(makeStepLabel(idx, step));

    updateUI();
}

void WidgetAnimation::onRemoveStepClicked()
{
    const int row = ui->listWidgetSteps->currentRow();
    if (row < 0 || row >= static_cast<int>(m_steps.size())) {
        return;
    }

    m_steps.erase(m_steps.begin() + row);

    // Rebuild the list to keep indices in sync
    ui->listWidgetSteps->clear();
    for (int i = 0; i < static_cast<int>(m_steps.size()); ++i) {
        ui->listWidgetSteps->addItem(makeStepLabel(i, m_steps[i]));
    }

    updateUI();
}

// Playback control
void WidgetAnimation::onPlayClicked()
{
    if (m_steps.empty()) {
        QMessageBox::information(this, tr("Info"), tr("No animation steps added yet."));
        return;
    }

    if (m_isPlaying) {
        return;
    }

    // speed 1 => 0.5 mm/tick, speed 10 => 5.0 mm/tick
    m_frameDistance = ui->sliderSpeed->value() * 0.5;

    m_isPlaying = true;
    ui->pushButtonPlay->setEnabled(false);
    ui->pushButtonPause->setEnabled(true);
    ui->labelStatus->setText(tr("Playing..."));

    m_timer->start();
}

void WidgetAnimation::onPauseClicked()
{
    if (!m_isPlaying) {
        return;
    }
    m_isPlaying = false;
    m_timer->stop();
    ui->pushButtonPlay->setEnabled(true);
    ui->pushButtonPause->setEnabled(false);
    ui->labelStatus->setText(tr("Paused."));
}

void WidgetAnimation::onRewindClicked()
{
    stopAnimation();

    // Restore every object to its original transform
    auto view = ViewManager::getInstance().getActiveView();
    for (auto &step : m_steps) {
        if (!step.object.IsNull()) {
            step.object->SetLocalTransformation(step.originalTrsf);
        }
    }

    if (view) {
        view->reDraw();
    }

    m_currentStep = 0;
    m_elapsed     = 0.0;

    ui->labelStatus->setText(tr("Rewound. Ready to play."));
    updateUI();
}

void WidgetAnimation::onCloseClicked()
{
    close();
}

void WidgetAnimation::onSpeedChanged(int value)
{
    ui->labelSpeedValue->setText(QString::number(value));
    if (m_isPlaying) {
        m_frameDistance = value * 0.5;
    }
}


// Timer tick (frame update)
void WidgetAnimation::onTimerTick()
{
    if (m_currentStep >= static_cast<int>(m_steps.size())) {
        stopAnimation();
        ui->labelStatus->setText(tr("Animation complete."));
        return;
    }

    applyCurrentFrame();
}

void WidgetAnimation::applyCurrentFrame()
{
    if (m_currentStep >= static_cast<int>(m_steps.size())) {
        return;
    }

    AnimationStep &step = m_steps[m_currentStep];
    if (step.object.IsNull()) {
        // Skip invalid objects
        ++m_currentStep;
        m_elapsed = 0.0;
        return;
    }

    const double remaining = step.distance - m_elapsed;
    const double delta     = qMin(m_frameDistance, remaining);

    // Apply incremental translation on top of the current local transform
    gp_Trsf move;
    move.SetTranslation(step.direction.Normalized() * delta);
    step.object->SetLocalTransformation(move * step.object->LocalTransformation());

    m_elapsed += delta;

    auto view = ViewManager::getInstance().getActiveView();
    if (view) {
        view->reDraw();
    }

    // Advance to the next step when the current one is complete
    if (m_elapsed >= step.distance - 1.0e-9) {
        ++m_currentStep;
        m_elapsed = 0.0;

        if (m_currentStep < static_cast<int>(m_steps.size())) {
            ui->labelStatus->setText(
                tr("Step %1 / %2 ...").arg(m_currentStep + 1).arg(m_steps.size()));
        }
    }
}


void WidgetAnimation::saveMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }
    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters   = view->getSelectionFilters();
}

void WidgetAnimation::restoreMouseState()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }
    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetAnimation::onObjectSelected);
    view->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
    for (const auto &filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, filter.second);
    }
    m_isPicking = false;
}

void WidgetAnimation::updateUI()
{
    ui->labelPickName->setText(m_pickedObject.IsNull() ? tr("(None)") : m_pickedName);

    ui->pushButtonAdd->setEnabled(!m_pickedObject.IsNull());

    ui->pushButtonPlay->setEnabled(!m_isPlaying && !m_steps.empty());
    ui->pushButtonPause->setEnabled(m_isPlaying);
    ui->pushButtonRewind->setEnabled(!m_steps.empty());
    ui->pushButtonRemove->setEnabled(!m_steps.empty());
}

gp_Vec WidgetAnimation::getSelectedDirection() const
{
    if (ui->radioButtonXPos->isChecked()) return gp_Vec( 1.0,  0.0,  0.0);
    if (ui->radioButtonXNeg->isChecked()) return gp_Vec(-1.0,  0.0,  0.0);
    if (ui->radioButtonYPos->isChecked()) return gp_Vec( 0.0,  1.0,  0.0);
    if (ui->radioButtonYNeg->isChecked()) return gp_Vec( 0.0, -1.0,  0.0);
    if (ui->radioButtonZPos->isChecked()) return gp_Vec( 0.0,  0.0,  1.0);
    if (ui->radioButtonZNeg->isChecked()) return gp_Vec( 0.0,  0.0, -1.0);
    return gp_Vec(1.0, 0.0, 0.0); // default fallback
}

void WidgetAnimation::stopAnimation()
{
    if (m_isPlaying) {
        m_timer->stop();
        m_isPlaying = false;
    }
    ui->pushButtonPlay->setEnabled(!m_steps.empty());
    ui->pushButtonPause->setEnabled(false);
}
