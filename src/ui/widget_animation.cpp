#include "widget_animation.h"
#include "ui_widget_animation.h"

#include "OCCView.h"
#include "SelectedEntity.h"
#include "ViewManager.h"

// OCC
#include <AIS_Shape.hxx>
#include <AIS_AnimationObject.hxx>
#include <AIS_Animation.hxx>
#include <AIS_InteractiveObject.hxx>
#include <TCollection_AsciiString.hxx>
#include <gp_Vec.hxx>
#include <gp_Trsf.hxx>

// Qt
#include <QMessageBox>
#include <QTimer>
#include <QListWidgetItem>
#include <QCloseEvent>

namespace
{
/// Returns a short display name for an AIS interactive object.
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

/// Converts a direction vector to a readable label string.
QString directionLabel(const gp_Vec &d)
{
    if (d.X() > 0.5)       return QStringLiteral("+X");
    if (d.X() < -0.5)      return QStringLiteral("-X");
    if (d.Y() > 0.5)       return QStringLiteral("+Y");
    if (d.Y() < -0.5)      return QStringLiteral("-Y");
    if (d.Z() > 0.5)       return QStringLiteral("+Z");
    return                         QStringLiteral("-Z");
}

/// Builds the list-widget display string for a step entry.
QString makeStepLabel(int index, const AnimationStep &step)
{
    return QString("[%1] %2  %3  %4 mm  %5 s")
        .arg(index + 1)
        .arg(step.objectName)
        .arg(directionLabel(step.direction))
        .arg(step.distance, 0, 'f', 2)
        .arg(step.duration, 0, 'f', 1);
}
} // namespace

WidgetAnimation::WidgetAnimation(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::WidgetAnimation),
      m_pollTimer(new QTimer(this)),
      m_rootAnimation(new AIS_Animation("AnimationRoot"))
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);

    m_pollTimer->setInterval(kPollIntervalMs);

    connect(ui->pushButtonPick,   &QPushButton::clicked,  this, &WidgetAnimation::onPickClicked);
    connect(ui->pushButtonAdd,    &QPushButton::clicked,  this, &WidgetAnimation::onAddStepClicked);
    connect(ui->pushButtonRemove, &QPushButton::clicked,  this, &WidgetAnimation::onRemoveStepClicked);
    connect(ui->pushButtonPlay,   &QPushButton::clicked,  this, &WidgetAnimation::onPlayClicked);
    connect(ui->pushButtonPause,  &QPushButton::clicked,  this, &WidgetAnimation::onPauseClicked);
    connect(ui->pushButtonRewind, &QPushButton::clicked,  this, &WidgetAnimation::onRewindClicked);
    connect(ui->pushButtonClose,  &QPushButton::clicked,  this, &WidgetAnimation::onCloseClicked);
    connect(m_pollTimer,          &QTimer::timeout,       this, &WidgetAnimation::onPollAnimation);
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
    stopPlayback();
    if (m_isPicking) {
        restoreMouseState();
    }
    QWidget::hide();
}

void WidgetAnimation::closeEvent(QCloseEvent *event)
{
    stopPlayback();
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

void WidgetAnimation::onObjectSelected(const TopoDS_Shape & /*shape*/)
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

    // Prefer the parent interactive object (whole solid / assembly node)
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
    // duration = distance / speed  (speed slider 1-10 maps to 10-100 mm/s)
    const double speedMmPerSec = ui->sliderSpeed->value() * 10.0;
    step.duration     = (speedMmPerSec > 0.0) ? (step.distance / speedMmPerSec) : kDefaultDuration;
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

    // Build (or rebuild) the OCC animation tree from the current step list
    buildAnimation();

    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }

    // StartTimer(startPts, playSpeed, toUpdateTimer, toLoop)
    m_rootAnimation->StartTimer(0.0, 1.0, true, false);

    m_isPlaying = true;
    m_pollTimer->start();

    ui->pushButtonPlay->setEnabled(false);
    ui->pushButtonPause->setEnabled(true);
    ui->labelStatus->setText(tr("Playing..."));
}

void WidgetAnimation::onPauseClicked()
{
    if (!m_isPlaying) {
        return;
    }

    m_rootAnimation->Pause();
    m_pollTimer->stop();
    m_isPlaying = false;

    ui->pushButtonPlay->setEnabled(true);
    ui->pushButtonPause->setEnabled(false);
    ui->labelStatus->setText(tr("Paused."));
}

void WidgetAnimation::onRewindClicked()
{
    stopPlayback();
    rewindToOriginal();

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
    // Speed change takes effect on the next Play (durations are baked into AnimationObjects)
}


void WidgetAnimation::onPollAnimation()
{
    if (!m_isPlaying) {
        return;
    }

    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }

    // Advance the OCC animation — AIS_AnimationObject::update() calls
    // myContext->SetLocation() internally to move the objects.
    m_rootAnimation->UpdateTimer();

    // IMPORTANT: OCCView is a QOpenGLWidget. V3d_View::Redraw() only works
    // when called from inside paintGL() (OpenGL context must be current).
    // Calling view->update() schedules paintGL() through Qt's event loop,
    // which then calls m_view->Redraw() with the correct context active.
    view->update();

    // Check completion: elapsed time has reached the total animation duration.
    // IsStopped() is unreliable here because the OCC timer keeps running
    // after the animation ends (it does not auto-stop for non-looping animations).
    const double elapsed = m_rootAnimation->ElapsedTime();
    const double total   = m_rootAnimation->Duration();
    if (total > 0.0 && elapsed >= total) {
        stopPlayback();
        ui->labelStatus->setText(tr("Animation complete."));
    }
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void WidgetAnimation::buildAnimation()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }

    // Clear all previous child animations and rebuild
    m_rootAnimation->Clear();

    double startPts = 0.0; // accumulated start time for sequential chaining

    for (int i = 0; i < static_cast<int>(m_steps.size()); ++i) {
        AnimationStep &step = m_steps[i];
        if (step.object.IsNull()) {
            continue;
        }

        // Start transform: current object pose (may have been modified by previous plays)
        const gp_Trsf &trsfStart = step.originalTrsf;

        // End transform: original pose + translation along direction
        gp_Trsf trsfEnd;
        trsfEnd.SetTranslation(step.direction.Normalized() * step.distance);
        // Compose: end = translation * original
        trsfEnd.Multiply(trsfStart);

        TCollection_AsciiString animName =
            TCollection_AsciiString("Step_") + TCollection_AsciiString(i + 1);

        Handle(AIS_AnimationObject) stepAnim =
            new AIS_AnimationObject(animName, view->Context(), step.object, trsfStart, trsfEnd);

        stepAnim->SetStartPts(startPts);
        stepAnim->SetOwnDuration(step.duration);

        m_rootAnimation->Add(stepAnim);

        startPts += step.duration;
    }
}

void WidgetAnimation::rewindToOriginal()
{
    auto view = ViewManager::getInstance().getActiveView();

    for (auto &step : m_steps) {
        if (!step.object.IsNull()) {
            step.object->SetLocalTransformation(step.originalTrsf);
        }
    }

    // Use view->update() for the same reason as onPollAnimation:
    // paintGL() must be the one to call m_view->Redraw().
    if (view) {
        view->update();
    }
}

void WidgetAnimation::stopPlayback()
{
    if (m_isPlaying) {
        m_rootAnimation->Stop();
        m_pollTimer->stop();
        m_isPlaying = false;
    }
    ui->pushButtonPlay->setEnabled(!m_steps.empty());
    ui->pushButtonPause->setEnabled(false);
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
