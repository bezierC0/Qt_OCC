#pragma once

#include <map>
#include <vector>

#include <TopAbs_ShapeEnum.hxx>

#include <AIS_InteractiveObject.hxx>
#include <gp_Vec.hxx>
#include <gp_Trsf.hxx>

#include <QWidget>

class QCloseEvent;
class QTimer;
class TopoDS_Shape;

namespace Ui
{
class WidgetAnimation;
}

/**
 * @brief Represents a single animation step.
 * Holds the target object, movement direction vector, total distance, and original transform.
 */
struct AnimationStep
{
    Handle(AIS_InteractiveObject) object;       // Target AIS interactive object
    QString                       objectName;   // Display name of the object
    gp_Vec                        direction;    // Unit vector representing movement direction
    double                        distance;     // Total movement distance [mm]
    gp_Trsf                       originalTrsf; // Original local transform, used for Rewind
};

/**
 * @brief Widget that applies translation animations to 3D objects.
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
    void onPickClicked();
    void onObjectSelected(const TopoDS_Shape &shape);
    void onAddStepClicked();
    void onRemoveStepClicked();
    void onPlayClicked();
    void onPauseClicked();
    void onRewindClicked();
    void onCloseClicked();
    void onTimerTick();
    void onSpeedChanged(int value);

private:
    // --- Helpers ---
    void   saveMouseState();
    void   restoreMouseState();
    void   updateUI();
    gp_Vec getSelectedDirection() const;
    void   stopAnimation();
    void   applyCurrentFrame();

private:
    Ui::WidgetAnimation *ui{};

    int                                 m_savedMouseMode{0};// Saved mouse/filter state for pick mode
    std::map<TopAbs_ShapeEnum, bool>    m_savedFilters{};
    bool                                m_isPicking{false};

    Handle(AIS_InteractiveObject)   m_pickedObject;// Currently picked object
    QString                         m_pickedName;

    std::vector<AnimationStep>      m_steps{};// List of animation steps

    // Playback control
    QTimer *m_timer{nullptr};
    bool    m_isPlaying{false};


    int    m_currentStep{0};     ///< Index of the step currently being executed
    double m_elapsed{0.0};       ///< Distance already moved within the current step [mm]
    double m_frameDistance{0.0}; ///< Movement amount per timer tick [mm]

    static constexpr int kTimerIntervalMs = 16; ///< ~60 fps
};
