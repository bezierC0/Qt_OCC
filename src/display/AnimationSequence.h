#pragma once

#include <vector>

#include <AIS_InteractiveObject.hxx>
#include <Graphic3d_Camera.hxx>
#include <QString>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

class AIS_InteractiveContext;

enum class AnimationStepType
{
    Translation,
    Rotation,
    Visibility,
    Camera
};

enum class AnimationEasing
{
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut
};

enum class AnimationStepTiming
{
    AfterPrevious,
    WithPrevious
};

double applyAnimationEasing(AnimationEasing easing, double progress);

struct AnimationStep
{
    Handle(AIS_InteractiveObject) object;
    QString                       objectName;
    AnimationStepType             type{AnimationStepType::Translation};
    gp_Vec                        direction;
    double                        distance{0.0};
    double                        rate{50.0};
    double                        duration{0.0};
    AnimationEasing               easing{AnimationEasing::Linear};
    AnimationStepTiming           timing{AnimationStepTiming::AfterPrevious};
    bool                          visible{true};
    Handle(Graphic3d_Camera)      cameraStart;
    Handle(Graphic3d_Camera)      cameraEnd;
    gp_Trsf                       originalTrsf;
};

struct AnimationSegment
{
    Handle(AIS_InteractiveObject) object;
    gp_Trsf                       startTransform;
    gp_Trsf                       endTransform;
    double                        startTime{0.0};
    double                        duration{0.0};
    AnimationEasing               easing{AnimationEasing::Linear};
    AnimationStepType             type{AnimationStepType::Translation};
    bool                          visible{true};
    Handle(Graphic3d_Camera)      cameraStart;
    Handle(Graphic3d_Camera)      cameraEnd;
};

/**
 * Owns animation-step rules independently from the dialog and OCCT timer.
 */
class AnimationSequence
{
public:
    bool empty() const { return m_steps.empty(); }
    int size() const { return static_cast<int>(m_steps.size()); }
    const std::vector<AnimationStep> &steps() const { return m_steps; }
    const AnimationStep *stepAt(int index) const;

    bool addStep(AnimationStep step);
    bool updateStep(int index, AnimationStepType type,
                    const gp_Vec &direction, double distance,
                    double rate, AnimationEasing easing,
                    AnimationStepTiming timing);
    bool updateStep(int index, AnimationStep step);
    bool removeStep(int index);
    bool moveStep(int from, int to);
    bool duplicateStep(int index);

    void setDefaultRate(double unitsPerSecond);
    double defaultRate() const { return m_defaultRatePerSecond; }
    double duration() const;

    std::vector<AnimationSegment> buildSegments() const;
    void restoreOriginalTransforms(const Handle(AIS_InteractiveContext) &context) const;

private:
    bool isValidIndex(int index) const;
    static bool hasParallelConflict(const std::vector<AnimationStep> &steps);
    bool prepareStep(AnimationStep &step, int replacedIndex = -1) const;
    static double durationFor(double distance, double rate);

private:
    std::vector<AnimationStep> m_steps;
    double                     m_defaultRatePerSecond{50.0};
};
