#pragma once

#include <map>

#include "AnimationSequence.h"

// OCC
#include <TopAbs_ShapeEnum.hxx>

// Qt
#include <QWidget>
#include <QPointer>
#include <Graphic3d_Camera.hxx>

class AIS_InteractiveObject;
class AIS_AnimationObject;
class AIS_Animation;
class TopoDS_Shape;

class QCloseEvent;
class QTimer;
class OCCView;

namespace Ui
{
class WidgetAnimation;
}

/**
 * @brief Animation dialog using OCC AIS_AnimationObject framework.
 *
 */
class WidgetAnimation : public QWidget
{
    Q_OBJECT

public:
    static WidgetAnimation *create(QWidget *parent = nullptr);
    explicit WidgetAnimation(QWidget *parent = nullptr);
    ~WidgetAnimation() override;

    void show();
    void hide();

signals:
    void redrawRequested();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void    onPickClicked();
    void    onObjectSelected(const TopoDS_Shape &shape);
    void    onAddStepClicked();
    void    onUpdateStepClicked();
    void    onRemoveStepClicked();
    void    onMoveStepUpClicked();
    void    onMoveStepDownClicked();
    void    onDuplicateStepClicked();
    void    onPreviewStepClicked();
    void    onStepSelectionChanged(int row);
    void    onPlayClicked();
    void    onPauseClicked();
    void    onRewindClicked();
    void    onCloseClicked();
    void    onSpeedChanged(int value);
    void    onStepTypeChanged(int index);
    void    onCaptureCameraStartClicked();
    void    onCaptureCameraEndClicked();
    void    onExportVideoClicked();
    void    onTimelineMoved(int value);
    void    onPollAnimation(); ///< Called by m_pollTimer to drive OCC animation updates

private:
    void    saveMouseState();
    void    restoreMouseState();
    void    updateUI();
    void    refreshStepList();
    gp_Vec  getSelectedDirection() const;
    void    setSelectedDirection(const gp_Vec &direction);
    AnimationStepType selectedStepType() const;
    void    setSelectedStepType(AnimationStepType type);
    AnimationEasing selectedEasing() const;
    void    setSelectedEasing(AnimationEasing easing);
    AnimationStepTiming selectedTiming() const;
    void    setSelectedTiming(AnimationStepTiming timing);
    void    buildAnimation();     ///< (Re)builds the OCCT timeline from m_sequence
    void    rewindToOriginal();   ///< Restores every object's transform to originalTrsf
    void    stopPlayback();       ///< Stops the OCC animation timer and poll timer
    void    prepareToClose();     ///< Stops playback and restores view before hide/destruction
    void    shutdown();           ///< One-time teardown used only by destruction
    void    invalidateTimeline();
    void    updateTimelineUI(double elapsed, double total);
    void    updateActiveSteps(double elapsed);
    void    applySceneTracks(double elapsed);

private:
    Ui::WidgetAnimation *ui{};

    int                                     m_savedMouseMode{0};
    std::map<TopAbs_ShapeEnum, bool>        m_savedFilters;
    bool                                    m_isPicking{false};
    QPointer<OCCView>                       m_pickView;
    QPointer<OCCView>                       m_playbackView;

    Handle(AIS_InteractiveObject)           m_pickedObject;// Currently picked object
    QString                                 m_pickedName{};

    AnimationSequence                       m_sequence;
    std::vector<AnimationSegment>            m_timelineSegments;

    Handle(AIS_Animation)                   m_rootAnimation;   ///< Root container for all step animations
    Handle(Graphic3d_Camera)                m_cameraStartCapture;
    Handle(Graphic3d_Camera)                m_cameraEndCapture;
    Handle(Graphic3d_Camera)                m_originalCamera;
    Handle(Graphic3d_Camera)                m_evaluatedCamera;
    std::map<const AIS_InteractiveObject *, std::pair<Handle(AIS_InteractiveObject), bool>>
                                            m_originalVisibility;
    QTimer                                  *m_pollTimer{nullptr};// Qt poll timer (drives OCC UpdateTimer + view repaint)
    bool                                    m_isPlaying{false};
    bool                                    m_isPaused{false};
    bool                                    m_isShutdown{false};
    bool                                    m_timelineDirty{true};
    double                                  m_previewTime{0.0};

    static constexpr int                    kPollIntervalMs  = 16;   ///< ~60 fps poll rate
    static constexpr int                    kTimelineMaximum = 1000;
};
