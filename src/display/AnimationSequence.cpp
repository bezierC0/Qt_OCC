#include "AnimationSequence.h"

#include <AIS_InteractiveContext.hxx>
#include <Precision.hxx>
#include <TopLoc_Location.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

double applyAnimationEasing(AnimationEasing easing, double progress)
{
    const double t = std::clamp(progress, 0.0, 1.0);
    switch (easing) {
    case AnimationEasing::EaseIn:
        return t * t;
    case AnimationEasing::EaseOut:
        return 1.0 - (1.0 - t) * (1.0 - t);
    case AnimationEasing::EaseInOut:
        return t < 0.5
            ? 2.0 * t * t
            : 1.0 - std::pow(-2.0 * t + 2.0, 2.0) / 2.0;
    case AnimationEasing::Linear:
    default:
        return t;
    }
}

const AnimationStep *AnimationSequence::stepAt(int index) const
{
    return isValidIndex(index) ? &m_steps[static_cast<std::size_t>(index)] : nullptr;
}

bool AnimationSequence::addStep(AnimationStep step)
{
    if (!prepareStep(step)) {
        return false;
    }
    if (m_steps.empty()) {
        step.timing = AnimationStepTiming::AfterPrevious;
    } else if (step.timing == AnimationStepTiming::WithPrevious) {
        std::vector<AnimationStep> candidate = m_steps;
        candidate.push_back(step);
        if (hasParallelConflict(candidate)) {
            return false;
        }
    }
    m_steps.push_back(std::move(step));
    return true;
}

bool AnimationSequence::updateStep(int index, AnimationStepType type,
                                   const gp_Vec &direction, double distance,
                                   double rate, AnimationEasing easing,
                                   AnimationStepTiming timing)
{
    if (!isValidIndex(index)
        || direction.SquareMagnitude() <= Precision::SquareConfusion()
        || std::abs(distance) <= Precision::Confusion()
        || rate <= Precision::Confusion()) {
        return false;
    }
    if (index == 0) {
        timing = AnimationStepTiming::AfterPrevious;
    }
    std::vector<AnimationStep> candidate = m_steps;
    AnimationStep &candidateStep = candidate[static_cast<std::size_t>(index)];
    candidateStep.timing = timing;
    if (hasParallelConflict(candidate)) {
        return false;
    }

    AnimationStep &step = m_steps[static_cast<std::size_t>(index)];
    step.type = type;
    step.direction = direction.Normalized();
    step.distance = distance;
    step.rate = rate;
    step.duration = durationFor(distance, rate);
    step.easing = easing;
    step.timing = timing;
    return true;
}

bool AnimationSequence::updateStep(int index, AnimationStep step)
{
    if (!isValidIndex(index) || !prepareStep(step, index)) {
        return false;
    }
    if (index == 0) {
        step.timing = AnimationStepTiming::AfterPrevious;
    }
    std::vector<AnimationStep> candidate = m_steps;
    candidate[static_cast<std::size_t>(index)] = step;
    if (hasParallelConflict(candidate)) {
        return false;
    }
    m_steps = std::move(candidate);
    return true;
}

bool AnimationSequence::removeStep(int index)
{
    if (!isValidIndex(index)) {
        return false;
    }
    const bool removedGroupLeader = m_steps[static_cast<std::size_t>(index)].timing
        == AnimationStepTiming::AfterPrevious;
    m_steps.erase(m_steps.begin() + index);
    if (!m_steps.empty()) {
        m_steps.front().timing = AnimationStepTiming::AfterPrevious;
        if (removedGroupLeader && index < size()) {
            m_steps[static_cast<std::size_t>(index)].timing = AnimationStepTiming::AfterPrevious;
        }
    }
    return true;
}

bool AnimationSequence::moveStep(int from, int to)
{
    if (!isValidIndex(from) || !isValidIndex(to) || from == to) {
        return false;
    }
    std::vector<AnimationStep> candidate = m_steps;
    if (from < to) {
        std::rotate(candidate.begin() + from,
                    candidate.begin() + from + 1,
                    candidate.begin() + to + 1);
    } else {
        std::rotate(candidate.begin() + to,
                    candidate.begin() + from,
                    candidate.begin() + from + 1);
    }
    candidate.front().timing = AnimationStepTiming::AfterPrevious;
    if (hasParallelConflict(candidate)) {
        return false;
    }
    m_steps = std::move(candidate);
    return true;
}

bool AnimationSequence::duplicateStep(int index)
{
    if (!isValidIndex(index)) {
        return false;
    }
    AnimationStep copy = m_steps[static_cast<std::size_t>(index)];
    copy.objectName += QStringLiteral(" Copy");
    copy.timing = AnimationStepTiming::AfterPrevious;
    m_steps.insert(m_steps.begin() + index + 1, std::move(copy));
    return true;
}

void AnimationSequence::setDefaultRate(double unitsPerSecond)
{
    if (unitsPerSecond > Precision::Confusion()) {
        m_defaultRatePerSecond = unitsPerSecond;
    }
}

double AnimationSequence::duration() const
{
    double groupStart = 0.0;
    double timelineEnd = 0.0;
    for (std::size_t i = 0; i < m_steps.size(); ++i) {
        const AnimationStep &step = m_steps[i];
        if (i == 0 || step.timing == AnimationStepTiming::AfterPrevious) {
            groupStart = timelineEnd;
        }
        timelineEnd = std::max(timelineEnd, groupStart + step.duration);
    }
    return timelineEnd;
}

std::vector<AnimationSegment> AnimationSequence::buildSegments() const
{
    std::vector<AnimationSegment> segments;
    segments.reserve(m_steps.size());
    std::unordered_map<const AIS_InteractiveObject *, gp_Trsf> currentTransforms;
    double groupStart = 0.0;
    double timelineEnd = 0.0;

    for (std::size_t i = 0; i < m_steps.size(); ++i) {
        const AnimationStep &step = m_steps[i];
        if (i == 0 || step.timing == AnimationStepTiming::AfterPrevious) {
            groupStart = timelineEnd;
        }
        if (step.type == AnimationStepType::Camera) {
            segments.push_back({Handle(AIS_InteractiveObject)(), gp_Trsf(), gp_Trsf(),
                                groupStart, step.duration, step.easing, step.type,
                                true, step.cameraStart, step.cameraEnd});
            timelineEnd = std::max(timelineEnd, groupStart + step.duration);
            continue;
        }
        if (step.object.IsNull()) {
            continue;
        }

        const AIS_InteractiveObject *objectKey = step.object.get();
        const auto current = currentTransforms.find(objectKey);
        const gp_Trsf startTransform = current == currentTransforms.end()
            ? step.originalTrsf
            : current->second;

        gp_Trsf endTransform;
        if (step.type == AnimationStepType::Translation) {
            endTransform.SetTranslation(step.direction * step.distance);
        } else if (step.type == AnimationStepType::Rotation) {
            const gp_Pnt origin(startTransform.TranslationPart());
            const double radians = step.distance * std::acos(-1.0) / 180.0;
            endTransform.SetRotation(gp_Ax1(origin, gp_Dir(step.direction)), radians);
        } else {
            endTransform = startTransform;
        }
        if (step.type == AnimationStepType::Translation
            || step.type == AnimationStepType::Rotation) {
            endTransform.Multiply(startTransform);
            currentTransforms[objectKey] = endTransform;
        }

        segments.push_back({step.object, startTransform, endTransform,
                            groupStart, step.duration, step.easing, step.type,
                            step.visible, step.cameraStart, step.cameraEnd});
        timelineEnd = std::max(timelineEnd, groupStart + step.duration);
    }
    return segments;
}

void AnimationSequence::restoreOriginalTransforms(
    const Handle(AIS_InteractiveContext) &context) const
{
    std::unordered_set<const AIS_InteractiveObject *> restoredObjects;
    for (const AnimationStep &step : m_steps) {
        const AIS_InteractiveObject *objectKey = step.object.get();
        if (!step.object.IsNull() && restoredObjects.insert(objectKey).second) {
            if (context.IsNull()) {
                step.object->SetLocalTransformation(step.originalTrsf);
            } else {
                // Notify AIS so both presentation and selection use the restored pose.
                context->SetLocation(step.object, TopLoc_Location(step.originalTrsf));
            }
        }
    }
}

bool AnimationSequence::isValidIndex(int index) const
{
    return index >= 0 && index < size();
}

bool AnimationSequence::hasParallelConflict(const std::vector<AnimationStep> &steps)
{
    std::unordered_set<const AIS_InteractiveObject *> groupObjects;
    bool hasCamera = false;
    for (std::size_t i = 0; i < steps.size(); ++i) {
        const AnimationStep &step = steps[i];
        if (i == 0 || step.timing == AnimationStepTiming::AfterPrevious) {
            groupObjects.clear();
            hasCamera = false;
        }
        if (step.type == AnimationStepType::Camera) {
            if (hasCamera) {
                return true;
            }
            hasCamera = true;
            continue;
        }
        const AIS_InteractiveObject *key = step.object.get();
        if (key != nullptr && !groupObjects.insert(key).second) {
            return true;
        }
    }
    return false;
}

bool AnimationSequence::prepareStep(AnimationStep &step, int replacedIndex) const
{
    if (step.rate <= Precision::Confusion()) {
        step.rate = m_defaultRatePerSecond;
    }

    if (step.type == AnimationStepType::Camera) {
        if (step.cameraStart.IsNull() || step.cameraEnd.IsNull()
            || step.duration <= Precision::Confusion()) {
            return false;
        }
        step.object.Nullify();
        step.objectName = QStringLiteral("Camera");
        return true;
    }

    if (step.object.IsNull()) {
        return false;
    }
    for (int i = 0; i < size(); ++i) {
        if (i != replacedIndex && m_steps[static_cast<std::size_t>(i)].object == step.object) {
            step.originalTrsf = m_steps[static_cast<std::size_t>(i)].originalTrsf;
            break;
        }
    }
    if (step.type == AnimationStepType::Visibility) {
        return step.duration > Precision::Confusion();
    }
    if (step.direction.SquareMagnitude() <= Precision::SquareConfusion()
        || std::abs(step.distance) <= Precision::Confusion()) {
        return false;
    }
    step.direction.Normalize();
    step.duration = durationFor(step.distance, step.rate);
    return true;
}

double AnimationSequence::durationFor(double distance, double rate)
{
    return std::abs(distance) / rate;
}
