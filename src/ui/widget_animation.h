#pragma once

#include <map>
#include <vector>

// OCC
#include <TopAbs_ShapeEnum.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

// Qt
#include <QWidget>

class AIS_InteractiveObject;
class AIS_AnimationObject;
class AIS_Animation;
class TopoDS_Shape;

class QCloseEvent;
class QTimer;

namespace Ui
{
class WidgetAnimation;
}

/**
 * @brief Describes one animation step: which object to move, in which direction, and how far.
 * The start/end transforms are computed at build time and stored in the AIS_AnimationObject.
 */
struct AnimationStep
{
    Handle(AIS_InteractiveObject) object;       ///< Target AIS interactive object
    QString                       objectName;   ///< Display name shown in the list
    gp_Vec                        direction;    ///< Unit direction vector (+X/-X/+Y/-Y/+Z/-Z)
    double                        distance;     ///< Translation distance [mm]
    double                        duration;     ///< Animation duration for this step [s]
    gp_Trsf                       originalTrsf; ///< Object transform before any animation (for Rewind)
};

/**
 * @brief Animation dialog using OCC AIS_AnimationObject framework.
 *
 */
class WidgetAnimation : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetAnimation(QWidget *parent = nullptr);
    ~WidgetAnimation() override;

    void show();
    void hide();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void    onPickClicked();
    void    onObjectSelected(const TopoDS_Shape &shape);
    void    onAddStepClicked();
    void    onRemoveStepClicked();
    void    onPlayClicked();
    void    onPauseClicked();
    void    onRewindClicked();
    void    onCloseClicked();
    void    onSpeedChanged(int value);
    void    onPollAnimation(); ///< Called by m_pollTimer to drive OCC animation updates

private:
    void    saveMouseState();
    void    restoreMouseState();
    void    updateUI();
    gp_Vec  getSelectedDirection() const;
    void    buildAnimation();     ///< (Re)builds the root AIS_Animation from m_steps
    void    rewindToOriginal();   ///< Restores every object's transform to originalTrsf
    void    stopPlayback();       ///< Stops the OCC animation timer and poll timer

private:
    Ui::WidgetAnimation *ui{};

    int                                     m_savedMouseMode{0};
    std::map<TopAbs_ShapeEnum, bool>        m_savedFilters;
    bool                                    m_isPicking{false};

    Handle(AIS_InteractiveObject)           m_pickedObject;// Currently picked object
    QString                                 m_pickedName{};

    std::vector<AnimationStep>              m_steps;    // Step list

    Handle(AIS_Animation)                   m_rootAnimation;   ///< Root container for all step animations
    QTimer                                  *m_pollTimer{nullptr};// Qt poll timer (drives OCC UpdateTimer + view repaint)
    bool                                    m_isPlaying{false};

    static constexpr int                    kPollIntervalMs  = 16;   ///< ~60 fps poll rate
    static constexpr double                 kDefaultDuration = 2.0;  ///< Default step duration [s]
};
