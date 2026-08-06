#include "widget_animation.h"
#include "ui_widget_animation.h"

#include "OCCView.h"
#include "SelectedEntity.h"
#include "TopoShapeUtil.h"
#include "ViewManager.h"

// OCC
#include <AIS_Shape.hxx>
#include <AIS_AnimationObject.hxx>
#include <AIS_Animation.hxx>
#include <AIS_InteractiveObject.hxx>
#include <Image_VideoRecorder.hxx>
#include <TCollection_AsciiString.hxx>
#include <gp_Vec.hxx>
#include <gp_Trsf.hxx>
#include <Precision.hxx>

// Qt
#include <QMessageBox>
#include <QTimer>
#include <QListWidgetItem>
#include <QCloseEvent>
#include <QSignalBlocker>
#include <QBrush>
#include <QColor>
#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QProgressDialog>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_map>

namespace
{
QString easingLabel(AnimationEasing easing)
{
    switch (easing) {
    case AnimationEasing::EaseIn:    return QStringLiteral("Ease In");
    case AnimationEasing::EaseOut:   return QStringLiteral("Ease Out");
    case AnimationEasing::EaseInOut: return QStringLiteral("Ease In/Out");
    case AnimationEasing::Linear:
    default:                         return QStringLiteral("Linear");
    }
}

class EasedAnimationObject : public AIS_AnimationObject
{
public:
    EasedAnimationObject(const TCollection_AsciiString &name,
                          const Handle(AIS_InteractiveContext) &context,
                          const Handle(AIS_InteractiveObject) &object,
                          const gp_Trsf &start, const gp_Trsf &end,
                          AnimationEasing easing)
        : AIS_AnimationObject(name, context, object, start, end),
          m_easing(easing)
    {
    }

protected:
    void update(const AIS_AnimationProgress &progress) override
    {
        AIS_AnimationProgress easedProgress = progress;
        easedProgress.LocalNormalized = applyAnimationEasing(
            m_easing, progress.LocalNormalized);
        easedProgress.LocalPts = easedProgress.LocalNormalized * OwnDuration();
        AIS_AnimationObject::update(easedProgress);
    }

private:
    AnimationEasing m_easing;
};

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
QString makeStepLabel(int index, const AnimationStep &step,
                      double startTime, double endTime)
{
    const QString timing = step.timing == AnimationStepTiming::WithPrevious
        ? QStringLiteral("Parallel") : QStringLiteral("After");
    if (step.type == AnimationStepType::Visibility) {
        return QString("[%1] %2  %3  Visibility=%4  %5-%6 s")
            .arg(index + 1).arg(step.objectName).arg(timing)
            .arg(step.visible ? QStringLiteral("Show") : QStringLiteral("Hide"))
            .arg(startTime, 0, 'f', 1).arg(endTime, 0, 'f', 1);
    }
    if (step.type == AnimationStepType::Camera) {
        return QString("[%1] Camera  %2  %3  %4-%5 s")
            .arg(index + 1).arg(timing).arg(easingLabel(step.easing))
            .arg(startTime, 0, 'f', 1).arg(endTime, 0, 'f', 1);
    }
    const bool isTranslation = step.type == AnimationStepType::Translation;
    return QString("[%1] %2  %3  %4 %5  %6 %7  %8/s  %9  %10-%11 s")
        .arg(index + 1)
        .arg(step.objectName)
        .arg(timing)
        .arg(isTranslation ? QStringLiteral("Move") : QStringLiteral("Rotate"))
        .arg(directionLabel(step.direction))
        .arg(step.distance, 0, 'f', 2)
        .arg(isTranslation ? QStringLiteral("mm") : QStringLiteral("deg"))
        .arg(step.rate, 0, 'f', 0)
        .arg(easingLabel(step.easing))
        .arg(startTime, 0, 'f', 1)
        .arg(endTime, 0, 'f', 1);
}
} // namespace

WidgetAnimation *WidgetAnimation::create(QWidget *parent)
{
    // Keep allocation beside the complete class implementation. This avoids a
    // stale caller object file allocating an outdated WidgetAnimation size.
    return new WidgetAnimation(parent);
}

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
    connect(ui->pushButtonUpdate, &QPushButton::clicked,  this, &WidgetAnimation::onUpdateStepClicked);
    connect(ui->pushButtonRemove, &QPushButton::clicked,  this, &WidgetAnimation::onRemoveStepClicked);
    connect(ui->pushButtonUp,     &QPushButton::clicked,  this, &WidgetAnimation::onMoveStepUpClicked);
    connect(ui->pushButtonDown,   &QPushButton::clicked,  this, &WidgetAnimation::onMoveStepDownClicked);
    connect(ui->pushButtonDuplicate, &QPushButton::clicked, this, &WidgetAnimation::onDuplicateStepClicked);
    connect(ui->pushButtonPreview, &QPushButton::clicked, this, &WidgetAnimation::onPreviewStepClicked);
    connect(ui->pushButtonPlay,   &QPushButton::clicked,  this, &WidgetAnimation::onPlayClicked);
    connect(ui->pushButtonPause,  &QPushButton::clicked,  this, &WidgetAnimation::onPauseClicked);
    connect(ui->pushButtonRewind, &QPushButton::clicked,  this, &WidgetAnimation::onRewindClicked);
    connect(ui->pushButtonClose,  &QPushButton::clicked,  this, &WidgetAnimation::onCloseClicked);
    connect(m_pollTimer,          &QTimer::timeout,       this, &WidgetAnimation::onPollAnimation);
    connect(ui->sliderSpeed,      &QSlider::valueChanged, this, &WidgetAnimation::onSpeedChanged);
    connect(ui->comboBoxStepType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &WidgetAnimation::onStepTypeChanged);
    connect(ui->pushButtonCameraStart, &QPushButton::clicked,
            this, &WidgetAnimation::onCaptureCameraStartClicked);
    connect(ui->pushButtonCameraEnd, &QPushButton::clicked,
            this, &WidgetAnimation::onCaptureCameraEndClicked);
    connect(ui->pushButtonExportVideo, &QPushButton::clicked,
            this, &WidgetAnimation::onExportVideoClicked);
    connect(ui->sliderTimeline, &QSlider::valueChanged,
            this, &WidgetAnimation::onTimelineMoved);
    connect(ui->listWidgetSteps,  &QListWidget::currentRowChanged,
            this, &WidgetAnimation::onStepSelectionChanged);

    onStepTypeChanged(ui->comboBoxStepType->currentIndex());
    onSpeedChanged(ui->sliderSpeed->value());
    updateUI();
}

WidgetAnimation::~WidgetAnimation()
{
    shutdown();
    delete ui;
}

/// Resolves the most specific target name available from OCAF/AIS metadata.
QString getTargetDisplayName(const View::SelectedEntity &entity,
                             const Handle(AIS_InteractiveObject) &object)
{
    TCollection_ExtendedString name = Util::Doc::GetNameFromLabel(entity.GetLabel());
    if (name.IsEmpty()) {
        name = Util::Ais::GetNameFromAISObject(object);
    }
    if (!name.IsEmpty()) {
        return QString::fromUtf16(
            reinterpret_cast<const ushort *>(name.ToExtString()));
    }
    return getObjectDisplayName(object);
}

void WidgetAnimation::show()
{
    QWidget::show();
    updateUI();
}

void WidgetAnimation::hide()
{
    prepareToClose();
    QWidget::hide();
}

void WidgetAnimation::closeEvent(QCloseEvent *event)
{
    // close() may come from the tool button, the title-bar button, or parent
    // window shutdown. Stop all playback callbacks before Qt hides/destroys us.
    prepareToClose();
    QWidget::closeEvent(event);
}

// Pick handling
void WidgetAnimation::onPickClicked()
{
    if (m_isPicking) {
        restoreMouseState();
        ui->labelStatus->setText(tr("Pick cancelled."));
        updateUI();
        return;
    }

    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        return;
    }

    m_pickView = view;
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

    auto view = m_pickView.data();
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

    m_pickedName = getTargetDisplayName(*entity, m_pickedObject);

    restoreMouseState();
    updateUI();
    ui->labelStatus->setText(tr("Selected target: %1").arg(m_pickedName));
}

// Step management
void WidgetAnimation::onAddStepClicked()
{
    const AnimationStepType type = selectedStepType();
    if (type != AnimationStepType::Camera && m_pickedObject.IsNull()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please pick a part first."));
        return;
    }

    AnimationStep step;
    step.object       = m_pickedObject;
    step.objectName   = type == AnimationStepType::Camera ? tr("Camera") : m_pickedName;
    step.type         = type;
    step.direction    = getSelectedDirection();
    step.distance     = ui->spinBoxDistance->value();
    step.rate         = ui->sliderSpeed->value() * 10.0;
    step.easing       = selectedEasing();
    step.timing       = selectedTiming();
    step.duration     = ui->spinBoxDuration->value();
    step.visible      = ui->comboBoxVisibility->currentIndex() == 0;
    step.cameraStart  = m_cameraStartCapture;
    step.cameraEnd    = m_cameraEndCapture;
    if ((type == AnimationStepType::Translation || type == AnimationStepType::Rotation)
        && std::abs(step.distance) <= Precision::Confusion()) {
        QMessageBox::warning(this, tr("Warning"), tr("Step amount must be non-zero."));
        return;
    }
    if (!m_pickedObject.IsNull()) {
        step.originalTrsf = m_pickedObject->LocalTransformation();
    }

    if (!m_sequence.addStep(std::move(step))) {
        QMessageBox::warning(this, tr("Warning"),
                             tr("Invalid action data or conflicting parallel track."));
        return;
    }

    invalidateTimeline();
    refreshStepList();
    updateUI();
}

void WidgetAnimation::onRemoveStepClicked()
{
    const int row = ui->listWidgetSteps->currentRow();
    if (!m_sequence.stepAt(row)) {
        return;
    }

    stopPlayback();
    rewindToOriginal();
    m_sequence.removeStep(row);

    invalidateTimeline();
    refreshStepList();
    updateUI();
}

void WidgetAnimation::onUpdateStepClicked()
{
    const int row = ui->listWidgetSteps->currentRow();
    if (!m_sequence.stepAt(row)) {
        return;
    }

    AnimationStep step = *m_sequence.stepAt(row);
    step.type = selectedStepType();
    step.direction = getSelectedDirection();
    step.distance = ui->spinBoxDistance->value();
    step.rate = ui->sliderSpeed->value() * 10.0;
    step.easing = selectedEasing();
    step.timing = selectedTiming();
    step.duration = ui->spinBoxDuration->value();
    step.visible = ui->comboBoxVisibility->currentIndex() == 0;
    if (step.type == AnimationStepType::Camera) {
        step.cameraStart = m_cameraStartCapture;
        step.cameraEnd = m_cameraEndCapture;
    } else if (step.object.IsNull() && !m_pickedObject.IsNull()) {
        step.object = m_pickedObject;
        step.objectName = m_pickedName;
        step.originalTrsf = m_pickedObject->LocalTransformation();
    }
    if ((step.type == AnimationStepType::Translation || step.type == AnimationStepType::Rotation)
        && std::abs(step.distance) <= Precision::Confusion()) {
        QMessageBox::warning(this, tr("Warning"), tr("Step amount must be non-zero."));
        return;
    }

    stopPlayback();
    rewindToOriginal();
    if (!m_sequence.updateStep(row, std::move(step))) {
        QMessageBox::warning(this, tr("Warning"),
                             tr("Invalid action data or conflicting parallel track."));
        return;
    }
    invalidateTimeline();
    refreshStepList();
    ui->listWidgetSteps->setCurrentRow(row);
    ui->progressBar->setValue(0);
    ui->labelStatus->setText(tr("Step updated."));
    updateUI();
}

void WidgetAnimation::onMoveStepUpClicked()
{
    const int row = ui->listWidgetSteps->currentRow();
    if (row <= 0 || !m_sequence.stepAt(row)) {
        return;
    }

    stopPlayback();
    rewindToOriginal();
    m_sequence.moveStep(row, row - 1);
    invalidateTimeline();
    refreshStepList();
    ui->listWidgetSteps->setCurrentRow(row - 1);
    ui->progressBar->setValue(0);
    updateUI();
}

void WidgetAnimation::onMoveStepDownClicked()
{
    const int row = ui->listWidgetSteps->currentRow();
    if (row < 0 || row + 1 >= m_sequence.size()) {
        return;
    }

    stopPlayback();
    rewindToOriginal();
    m_sequence.moveStep(row, row + 1);
    invalidateTimeline();
    refreshStepList();
    ui->listWidgetSteps->setCurrentRow(row + 1);
    ui->progressBar->setValue(0);
    updateUI();
}

void WidgetAnimation::onDuplicateStepClicked()
{
    const int row = ui->listWidgetSteps->currentRow();
    if (!m_sequence.duplicateStep(row)) {
        return;
    }
    stopPlayback();
    rewindToOriginal();
    invalidateTimeline();
    refreshStepList();
    ui->listWidgetSteps->setCurrentRow(row + 1);
    ui->labelStatus->setText(tr("Step duplicated."));
    updateUI();
}

void WidgetAnimation::onPreviewStepClicked()
{
    const int row = ui->listWidgetSteps->currentRow();
    if (m_timelineSegments.size() != static_cast<std::size_t>(m_sequence.size())) {
        m_timelineSegments = m_sequence.buildSegments();
    }
    if (row < 0 || row >= static_cast<int>(m_timelineSegments.size())) {
        return;
    }
    const double total = m_sequence.duration();
    const int value = total > 0.0
        ? static_cast<int>(m_timelineSegments[static_cast<std::size_t>(row)].startTime
                           / total * kTimelineMaximum)
        : 0;
    if (ui->sliderTimeline->value() == value) {
        onTimelineMoved(value);
    } else {
        ui->sliderTimeline->setValue(value);
    }
}

void WidgetAnimation::onStepSelectionChanged(int row)
{
    if (const AnimationStep *step = m_sequence.stepAt(row)) {
        ui->spinBoxDistance->setValue(step->distance);
        setSelectedDirection(step->direction);
        setSelectedStepType(step->type);
        ui->sliderSpeed->setValue(static_cast<int>(std::round(step->rate / 10.0)));
        setSelectedEasing(step->easing);
        setSelectedTiming(step->timing);
        ui->spinBoxDuration->setValue(step->duration);
        ui->comboBoxVisibility->setCurrentIndex(step->visible ? 0 : 1);
        if (step->type == AnimationStepType::Camera) {
            m_cameraStartCapture = step->cameraStart;
            m_cameraEndCapture = step->cameraEnd;
        }
        ui->labelCameraCapture->setText(tr("Start: %1 / End: %2")
            .arg(m_cameraStartCapture.IsNull() ? tr("No") : tr("Yes"))
            .arg(m_cameraEndCapture.IsNull() ? tr("No") : tr("Yes")));
    }
    updateUI();
}

// Playback control
void WidgetAnimation::onPlayClicked()
{
    if (m_sequence.empty()) {
        QMessageBox::information(this, tr("Info"), tr("No animation steps added yet."));
        return;
    }

    auto view = (m_isPaused || m_previewTime > 0.0)
        ? m_playbackView.data()
        : ViewManager::getInstance().getActiveView();
    if (!view) {
        stopPlayback();
        ui->labelStatus->setText(tr("Playback stopped: no active view."));
        return;
    }

    if (m_isPaused) {
        m_rootAnimation->Start(false);
    } else {
        m_playbackView = view;
        if (m_previewTime <= 0.0) {
            rewindToOriginal();
        }
        if (m_timelineDirty) {
            buildAnimation();
        }
        m_rootAnimation->StartTimer(m_previewTime, 1.0, true, false);
    }

    m_isPlaying = true;
    m_isPaused = false;
    m_pollTimer->start();

    ui->labelStatus->setText(tr("Playing..."));
    updateUI();
}

void WidgetAnimation::onPauseClicked()
{
    if (!m_isPlaying) {
        return;
    }

    m_rootAnimation->Pause();
    m_pollTimer->stop();
    m_previewTime = m_rootAnimation->ElapsedTime();
    m_isPlaying = false;
    m_isPaused = true;

    ui->labelStatus->setText(tr("Paused."));
    updateUI();
}

void WidgetAnimation::onRewindClicked()
{
    stopPlayback();
    rewindToOriginal();
    m_previewTime = 0.0;
    updateTimelineUI(0.0, m_sequence.duration());

    ui->labelStatus->setText(tr("Rewound. Ready to play."));
    updateUI();
}

void WidgetAnimation::onCloseClicked()
{
    close();
}

void WidgetAnimation::onExportVideoClicked()
{
    if (m_sequence.empty()) {
        QMessageBox::information(this, tr("Info"), tr("No animation steps to export."));
        return;
    }
    QString outputFile = QFileDialog::getSaveFileName(
        this, tr("Export Animation Video"), QString(),
        tr("MP4 Video (*.mp4)"));
    if (outputFile.isEmpty()) {
        return;
    }
    if (!outputFile.endsWith(QStringLiteral(".mp4"), Qt::CaseInsensitive)) {
        outputFile += QStringLiteral(".mp4");
    }

    auto view = ViewManager::getInstance().getActiveView();
    if (!view) {
        QMessageBox::warning(this, tr("Warning"), tr("No active view to capture."));
        return;
    }

    stopPlayback();
    rewindToOriginal();
    m_playbackView = view;
    buildAnimation();

    const int fps = ui->spinBoxVideoFps->value();
    const double total = m_sequence.duration();
    const int frameCount = std::max(1, static_cast<int>(std::round(total * fps)));
    QProgressDialog progress(tr("Rendering video frames..."), tr("Cancel"),
                             0, frameCount, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);

    bool framesReady = true;
    QString exportError;
    Handle(Image_VideoRecorder) recorder = new Image_VideoRecorder();
    bool recorderOpened = false;
    int videoWidth = 0;
    int videoHeight = 0;
    // AIS_Animation::Update() only evaluates a timeline in Started state.
    // Real-time playback enters this state through StartTimer().
    m_rootAnimation->Start(false);
    for (int frame = 0; frame < frameCount; ++frame) {
        if (progress.wasCanceled()) {
            framesReady = false;
            break;
        }
        const double time = frameCount > 1
            ? total * frame / static_cast<double>(frameCount - 1)
            : total;
        m_rootAnimation->Update(time);
        applySceneTracks(time);
        view->requestSceneRedraw();
        view->repaint();
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        QImage image = view->grabFramebuffer().convertToFormat(QImage::Format_RGBA8888);
        if (image.isNull()) {
            framesReady = false;
            exportError = tr("Could not capture video frame %1.").arg(frame);
            break;
        }

        if (!recorderOpened) {
            // yuv420p requires even dimensions. Crop at most one pixel rather
            // than scaling the viewport and changing the captured geometry.
            videoWidth = image.width() & ~1;
            videoHeight = image.height() & ~1;
            if (videoWidth <= 0 || videoHeight <= 0) {
                framesReady = false;
                exportError = tr("The captured view is too small for video export.");
                break;
            }
            Image_VideoParams params;
            params.Format = "mp4";
            params.PixelFormat = "yuv420p";
            params.Width = videoWidth;
            params.Height = videoHeight;
            params.SetFramerate(fps);
            const QByteArray encodedPath = QFile::encodeName(outputFile);
            recorderOpened = recorder->Open(encodedPath.constData(), params);
            if (!recorderOpened) {
                framesReady = false;
                exportError = tr("OpenCASCADE could not initialize the MP4 recorder. "
                                 "Check that the OCCT FFmpeg runtime DLLs are available.");
                break;
            }
        }

        if (image.width() != videoWidth || image.height() != videoHeight) {
            image = image.copy(0, 0, videoWidth, videoHeight);
        }
        Image_PixMap &videoFrame = recorder->ChangeFrame();
        const std::size_t rowBytes = static_cast<std::size_t>(videoWidth) * 4;
        for (int row = 0; row < videoHeight; ++row) {
            std::memcpy(videoFrame.ChangeRow(static_cast<Standard_Size>(row)),
                        image.constScanLine(row), rowBytes);
        }
        if (!recorder->PushFrame()) {
            framesReady = false;
            exportError = tr("OpenCASCADE failed while encoding video frame %1.").arg(frame);
            break;
        }
        progress.setValue(frame + 1);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }

    m_rootAnimation->Stop();
    progress.setLabelText(tr("Finalizing MP4 video..."));
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    if (recorderOpened) {
        recorder->Close();
    }
    rewindToOriginal();
    m_previewTime = 0.0;
    updateTimelineUI(0.0, total);
    if (!framesReady) {
        QFile::remove(outputFile);
        if (progress.wasCanceled()) {
            ui->labelStatus->setText(tr("Video export cancelled."));
        } else {
            ui->labelStatus->setText(tr("Video export failed."));
            QMessageBox::critical(this, tr("Export failed"), exportError);
        }
        return;
    }

    progress.close();
    ui->labelStatus->setText(tr("Video exported: %1").arg(outputFile));
    QMessageBox::information(this, tr("Export complete"),
                             tr("Video saved to:\n%1").arg(outputFile));
}

void WidgetAnimation::onSpeedChanged(int value)
{
    const double rate = value * 10.0;
    ui->labelSpeedValue->setText(QString::number(rate));
}

void WidgetAnimation::onStepTypeChanged(int /*index*/)
{
    const AnimationStepType type = selectedStepType();
    const bool isTransform = type == AnimationStepType::Translation
        || type == AnimationStepType::Rotation;
    const bool isVisibility = type == AnimationStepType::Visibility;
    const bool isCamera = type == AnimationStepType::Camera;

    ui->labelMove->setVisible(isTransform);
    ui->spinBoxDistance->setVisible(isTransform);
    ui->groupBoxDir->setVisible(isTransform);
    ui->labelSpeed->setVisible(isTransform);
    ui->sliderSpeed->setVisible(isTransform);
    ui->labelSpeedValue->setVisible(isTransform);
    ui->labelDuration->setVisible(!isTransform);
    ui->spinBoxDuration->setVisible(!isTransform);
    ui->labelVisibility->setVisible(isVisibility);
    ui->comboBoxVisibility->setVisible(isVisibility);
    ui->pushButtonCameraStart->setVisible(isCamera);
    ui->pushButtonCameraEnd->setVisible(isCamera);
    ui->labelCameraCapture->setVisible(isCamera);
    ui->labelEasing->setVisible(!isVisibility);
    ui->comboBoxEasing->setVisible(!isVisibility);

    if (isTransform) {
        const bool isTranslation = type == AnimationStepType::Translation;
        ui->labelMove->setText(isTranslation ? tr("Distance (mm):") : tr("Angle (deg):"));
        ui->groupBoxDir->setTitle(isTranslation ? tr("Direction") : tr("Axis"));
    }
    updateUI();
}

void WidgetAnimation::onCaptureCameraStartClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view || view->ActiveView().IsNull()) return;
    m_cameraStartCapture = new Graphic3d_Camera();
    m_cameraStartCapture->Copy(view->ActiveView()->Camera());
    ui->labelCameraCapture->setText(tr("Start: Yes / End: %1")
        .arg(m_cameraEndCapture.IsNull() ? tr("No") : tr("Yes")));
    updateUI();
}

void WidgetAnimation::onCaptureCameraEndClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view || view->ActiveView().IsNull()) return;
    m_cameraEndCapture = new Graphic3d_Camera();
    m_cameraEndCapture->Copy(view->ActiveView()->Camera());
    ui->labelCameraCapture->setText(tr("Start: %1 / End: Yes")
        .arg(m_cameraStartCapture.IsNull() ? tr("No") : tr("Yes")));
    updateUI();
}

void WidgetAnimation::onTimelineMoved(int value)
{
    if (m_sequence.empty()) {
        return;
    }

    auto view = m_playbackView.data();
    if (!view) {
        view = ViewManager::getInstance().getActiveView();
        m_playbackView = view;
    }
    if (!view) {
        return;
    }

    if (m_isPlaying || m_isPaused) {
        m_rootAnimation->Stop();
        m_pollTimer->stop();
        m_isPlaying = false;
        m_isPaused = false;
    }
    if (m_timelineDirty) {
        rewindToOriginal();
        buildAnimation();
    }

    const double total = m_rootAnimation->Duration();
    m_previewTime = total * value / static_cast<double>(kTimelineMaximum);
    m_rootAnimation->Update(m_previewTime);
    applySceneTracks(m_previewTime);
    view->update();
    updateTimelineUI(m_previewTime, total);
    ui->labelStatus->setText(tr("Previewing %1 s").arg(m_previewTime, 0, 'f', 2));
    updateUI();
}


void WidgetAnimation::onPollAnimation()
{
    if (!m_isPlaying) {
        return;
    }

    auto view = m_playbackView.data();
    if (!view) {
        stopPlayback();
        ui->labelStatus->setText(tr("Playback stopped: source view was closed."));
        return;
    }

    // Advance the OCC animation - AIS_AnimationObject::update() calls
    // myContext->SetLocation() internally to move the objects.
    m_rootAnimation->UpdateTimer();
    applySceneTracks(m_rootAnimation->ElapsedTime());

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
    m_previewTime = elapsed;
    if (total > 0.0) {
        updateTimelineUI(elapsed, total);
    }
    updateActiveSteps(elapsed);
    if (total > 0.0 && elapsed >= total) {
        if (ui->checkBoxLoop->isChecked()) {
            m_rootAnimation->StartTimer(0.0, 1.0, true, false);
            m_previewTime = 0.0;
            updateTimelineUI(0.0, total);
            return;
        }
        stopPlayback();
        updateTimelineUI(total, total);
        m_previewTime = 0.0;
        ui->labelStatus->setText(tr("Animation complete."));
    }
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void WidgetAnimation::buildAnimation()
{
    auto view = m_playbackView.data();
    if (!view) {
        return;
    }

    // Clear all previous child animations and rebuild
    m_rootAnimation->Clear();
    m_rootAnimation->SetOwnDuration(m_sequence.duration());

    m_timelineSegments = m_sequence.buildSegments();
    std::unordered_map<const AIS_InteractiveObject *, bool> currentVisibility;
    for (int i = 0; i < static_cast<int>(m_timelineSegments.size()); ++i) {
        const AnimationSegment &segment = m_timelineSegments[static_cast<std::size_t>(i)];
        TCollection_AsciiString animName =
            TCollection_AsciiString("Step_") + TCollection_AsciiString(i + 1);

        Handle(AIS_Animation) stepAnim;
        if (segment.type == AnimationStepType::Camera) {
            if (m_originalCamera.IsNull()) {
                m_originalCamera = new Graphic3d_Camera();
                m_originalCamera->Copy(segment.cameraStart);
            }
            continue;
        } else if (segment.type == AnimationStepType::Visibility) {
            const AIS_InteractiveObject *key = segment.object.get();
            auto current = currentVisibility.find(key);
            bool startVisible;
            if (current == currentVisibility.end()) {
                startVisible = view->Context()->IsDisplayed(segment.object);
                currentVisibility[key] = startVisible;
                m_originalVisibility.emplace(
                    key, std::make_pair(segment.object, startVisible));
            } else {
                startVisible = current->second;
            }
            currentVisibility[key] = segment.visible;
            continue;
        } else {
            stepAnim = new EasedAnimationObject(animName, view->Context(), segment.object,
                                                segment.startTransform, segment.endTransform,
                                                segment.easing);
        }

        stepAnim->SetStartPts(segment.startTime);
        stepAnim->SetOwnDuration(segment.duration);

        m_rootAnimation->Add(stepAnim);
    }
    m_timelineDirty = false;
}

void WidgetAnimation::rewindToOriginal()
{
    auto view = m_playbackView.data();
    if (!view) {
        view = ViewManager::getInstance().getActiveView();
    }
    const Handle(AIS_InteractiveContext) context = view
        ? view->Context()
        : Handle(AIS_InteractiveContext)();
    m_sequence.restoreOriginalTransforms(context);

    if (!context.IsNull()) {
        for (const auto &entry : m_originalVisibility) {
            const Handle(AIS_InteractiveObject) &object = entry.second.first;
            const bool visible = entry.second.second;
            if (visible) context->Display(object, false);
            else         context->Erase(object, false);
        }
    }
    m_originalVisibility.clear();

    if (view && !m_originalCamera.IsNull() && !view->ActiveView().IsNull()) {
        view->ActiveView()->SetCamera(m_originalCamera);
        view->requestSceneRedraw();
    }
    m_originalCamera.Nullify();

    // The non-modal tool asks its owner to redraw after the close event returns.
    if (view) {
        emit redrawRequested();
    }
}

void WidgetAnimation::stopPlayback()
{
    // Always stop the poll timer first. Its timeout calls UpdateTimer() and
    // must not run while the widget or OCCT animation tree is being closed.
    if (m_pollTimer) {
        m_pollTimer->stop();
    }
    if ((m_isPlaying || m_isPaused) && !m_rootAnimation.IsNull()) {
        m_rootAnimation->Stop();
    }
    m_isPlaying = false;
    m_isPaused = false;
    updateActiveSteps(-1.0);
    if (!m_isShutdown) {
        updateUI();
    }
}

void WidgetAnimation::prepareToClose()
{
    stopPlayback();
    if (m_isPicking) {
        restoreMouseState();
    }
    rewindToOriginal();
}

void WidgetAnimation::shutdown()
{
    if (m_isShutdown) {
        return;
    }
    m_isShutdown = true;

    // Prevent another poll callback while OCCT animation children are released.
    if (m_pollTimer) {
        m_pollTimer->stop();
        disconnect(m_pollTimer, &QTimer::timeout,
                   this, &WidgetAnimation::onPollAnimation);
    }
    prepareToClose();

    // Destroy OCCT animation children before QWidget and generated UI teardown.
    if (!m_rootAnimation.IsNull()) {
        m_rootAnimation->Clear();
    }
    m_rootAnimation.Nullify();
}

void WidgetAnimation::invalidateTimeline()
{
    m_timelineDirty = true;
    m_timelineSegments.clear();
    m_previewTime = 0.0;
    updateTimelineUI(0.0, m_sequence.duration());
}

void WidgetAnimation::updateTimelineUI(double elapsed, double total)
{
    const double ratio = total > 0.0
        ? std::clamp(elapsed / total, 0.0, 1.0)
        : 0.0;
    const QSignalBlocker blocker(ui->sliderTimeline);
    ui->sliderTimeline->setValue(static_cast<int>(ratio * kTimelineMaximum));
    ui->progressBar->setValue(static_cast<int>(ratio * 100.0));
    ui->labelTimelineValue->setText(
        tr("%1 / %2 s").arg(elapsed, 0, 'f', 1).arg(total, 0, 'f', 1));
}

void WidgetAnimation::saveMouseState()
{
    auto view = m_pickView.data();
    if (!view) {
        m_isPicking = false;
        m_pickView.clear();
        return;
    }
    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters   = view->getSelectionFilters();
}

void WidgetAnimation::restoreMouseState()
{
    auto view = m_pickView.data();
    if (!view) {
        m_isPicking = false;
        m_pickView.clear();
        return;
    }
    disconnect(view, &OCCView::signalSpaceSelected, this, &WidgetAnimation::onObjectSelected);
    view->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
    for (const auto &filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, filter.second);
    }
    m_isPicking = false;
    m_pickView.clear();
}

void WidgetAnimation::refreshStepList()
{
    const int selectedRow = ui->listWidgetSteps->currentRow();
    ui->listWidgetSteps->clear();
    const std::vector<AnimationStep> &steps = m_sequence.steps();
    m_timelineSegments = m_sequence.buildSegments();
    for (int i = 0; i < static_cast<int>(steps.size()); ++i) {
        const AnimationSegment &segment = m_timelineSegments[static_cast<std::size_t>(i)];
        ui->listWidgetSteps->addItem(makeStepLabel(
            i, steps[static_cast<std::size_t>(i)], segment.startTime,
            segment.startTime + segment.duration));
    }
    if (selectedRow >= 0 && selectedRow < m_sequence.size()) {
        ui->listWidgetSteps->setCurrentRow(selectedRow);
    }
}

void WidgetAnimation::updateUI()
{
    ui->labelPickName->setText(m_pickedObject.IsNull() ? tr("(None)") : m_pickedName);

    const int selectedRow = ui->listWidgetSteps->currentRow();
    const bool hasSelection = selectedRow >= 0
        && selectedRow < m_sequence.size();
    const bool canEdit = !m_isPlaying && !m_isPaused && !m_isPicking;
    const AnimationStepType type = selectedStepType();
    const bool isCamera = type == AnimationStepType::Camera;
    const bool cameraReady = !m_cameraStartCapture.IsNull() && !m_cameraEndCapture.IsNull();

    ui->pushButtonPick->setText(m_isPicking ? tr("Cancel") : tr("Pick"));
    ui->pushButtonAdd->setEnabled(canEdit
        && (isCamera ? cameraReady : !m_pickedObject.IsNull()));
    ui->pushButtonPick->setEnabled(!isCamera && !m_isPlaying && !m_isPaused);
    ui->pushButtonPlay->setEnabled(!m_isPicking && !m_isPlaying && !m_sequence.empty());
    ui->pushButtonPause->setEnabled(m_isPlaying);
    ui->pushButtonRewind->setEnabled(!m_isPicking && !m_sequence.empty());
    ui->pushButtonUpdate->setEnabled(hasSelection && canEdit);
    ui->pushButtonRemove->setEnabled(hasSelection && canEdit);
    ui->pushButtonUp->setEnabled(hasSelection && selectedRow > 0 && canEdit);
    ui->pushButtonDown->setEnabled(hasSelection
        && selectedRow + 1 < m_sequence.size() && canEdit);
    ui->pushButtonDuplicate->setEnabled(hasSelection && canEdit);
    ui->pushButtonPreview->setEnabled(hasSelection && !m_isPlaying && !m_isPicking);
    ui->sliderSpeed->setEnabled(canEdit);
    ui->comboBoxEasing->setEnabled(canEdit);
    ui->comboBoxTiming->setEnabled(canEdit && selectedRow != 0);
    ui->comboBoxStepType->setEnabled(canEdit);
    ui->spinBoxDuration->setEnabled(canEdit);
    ui->comboBoxVisibility->setEnabled(canEdit);
    ui->pushButtonCameraStart->setEnabled(canEdit);
    ui->pushButtonCameraEnd->setEnabled(canEdit);
    ui->spinBoxDistance->setEnabled(canEdit);
    ui->groupBoxDir->setEnabled(canEdit);
    ui->checkBoxLoop->setEnabled(!m_isPaused);
    ui->sliderTimeline->setEnabled(!m_isPicking && !m_isPlaying && !m_sequence.empty());
    ui->pushButtonExportVideo->setEnabled(canEdit && !m_sequence.empty());
    ui->spinBoxVideoFps->setEnabled(canEdit);
}

void WidgetAnimation::setSelectedDirection(const gp_Vec &direction)
{
    if (direction.X() > 0.5)       ui->radioButtonXPos->setChecked(true);
    else if (direction.X() < -0.5) ui->radioButtonXNeg->setChecked(true);
    else if (direction.Y() > 0.5)  ui->radioButtonYPos->setChecked(true);
    else if (direction.Y() < -0.5) ui->radioButtonYNeg->setChecked(true);
    else if (direction.Z() > 0.5)  ui->radioButtonZPos->setChecked(true);
    else                           ui->radioButtonZNeg->setChecked(true);
}

AnimationStepType WidgetAnimation::selectedStepType() const
{
    return static_cast<AnimationStepType>(ui->comboBoxStepType->currentIndex());
}

void WidgetAnimation::setSelectedStepType(AnimationStepType type)
{
    ui->comboBoxStepType->setCurrentIndex(static_cast<int>(type));
}

AnimationEasing WidgetAnimation::selectedEasing() const
{
    switch (ui->comboBoxEasing->currentIndex()) {
    case 1:  return AnimationEasing::EaseIn;
    case 2:  return AnimationEasing::EaseOut;
    case 3:  return AnimationEasing::EaseInOut;
    default: return AnimationEasing::Linear;
    }
}

void WidgetAnimation::setSelectedEasing(AnimationEasing easing)
{
    ui->comboBoxEasing->setCurrentIndex(static_cast<int>(easing));
}

AnimationStepTiming WidgetAnimation::selectedTiming() const
{
    return ui->comboBoxTiming->currentIndex() == 1
        ? AnimationStepTiming::WithPrevious
        : AnimationStepTiming::AfterPrevious;
}

void WidgetAnimation::setSelectedTiming(AnimationStepTiming timing)
{
    ui->comboBoxTiming->setCurrentIndex(
        timing == AnimationStepTiming::WithPrevious ? 1 : 0);
}

void WidgetAnimation::updateActiveSteps(double elapsed)
{
    for (int i = 0; i < ui->listWidgetSteps->count(); ++i) {
        QListWidgetItem *item = ui->listWidgetSteps->item(i);
        if (i >= static_cast<int>(m_timelineSegments.size())) {
            item->setBackground(QBrush());
            continue;
        }
        const AnimationSegment &segment = m_timelineSegments[static_cast<std::size_t>(i)];
        const bool active = m_isPlaying
            && elapsed >= segment.startTime
            && elapsed < segment.startTime + segment.duration;
        item->setBackground(active ? QBrush(QColor(255, 235, 150)) : QBrush());
    }
}

void WidgetAnimation::applySceneTracks(double elapsed)
{
    auto view = m_playbackView.data();
    if (!view || view->Context().IsNull() || view->ActiveView().IsNull()) {
        return;
    }
    bool sceneChanged = false;

    // Compute the final visibility from the captured baseline first. Applying
    // only the final state avoids Display/Erase churn on every timer tick.
    std::unordered_map<const AIS_InteractiveObject *, bool> desiredVisibility;
    for (const auto &entry : m_originalVisibility) {
        desiredVisibility[entry.first] = entry.second.second;
    }
    for (const AnimationSegment &segment : m_timelineSegments) {
        if (segment.type != AnimationStepType::Visibility
            || elapsed < segment.startTime + segment.duration * 0.5) {
            continue;
        }
        desiredVisibility[segment.object.get()] = segment.visible;
    }
    for (const auto &entry : m_originalVisibility) {
        const Handle(AIS_InteractiveObject) &object = entry.second.first;
        const bool visible = desiredVisibility[entry.first];
        if (visible != static_cast<bool>(view->Context()->IsDisplayed(object))) {
            if (visible) view->Context()->Display(object, false);
            else         view->Context()->Erase(object, false);
            sceneChanged = true;
        }
    }

    if (m_originalCamera.IsNull()) {
        if (sceneChanged) {
            view->requestSceneRedraw();
        }
        return;
    }
    if (m_evaluatedCamera.IsNull()) {
        m_evaluatedCamera = new Graphic3d_Camera();
    }
    m_evaluatedCamera->Copy(m_originalCamera);
    for (const AnimationSegment &segment : m_timelineSegments) {
        if (segment.type != AnimationStepType::Camera || elapsed < segment.startTime) {
            continue;
        }
        const double local = segment.duration > 0.0
            ? std::clamp((elapsed - segment.startTime) / segment.duration, 0.0, 1.0)
            : 1.0;
        const double eased = applyAnimationEasing(segment.easing, local);
        Graphic3d_Camera::Interpolate(segment.cameraStart, segment.cameraEnd,
                                      eased, m_evaluatedCamera);
        if (local < 1.0) {
            break;
        }
    }
    view->ActiveView()->SetCamera(m_evaluatedCamera);
    view->requestSceneRedraw();
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
