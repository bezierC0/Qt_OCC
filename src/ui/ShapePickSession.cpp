#include "ShapePickSession.h"
#include "ViewerPickHelper.h"

ShapePickSession::ShapePickSession(int            requiredPointCount,
                                   PreviewBuilder previewBuilder,
                                   QObject*       parent)
    : QObject(parent)
    , m_requiredPointCount(requiredPointCount)
    , m_previewBuilder(std::move(previewBuilder))
{
    // ViewerPickHelper is owned by this object; its lifetime is tied to the session.
    m_helper = new ViewerPickHelper(this);

    connect(m_helper, &ViewerPickHelper::pointPicked,
            this,     &ShapePickSession::onPointPicked);

    connect(m_helper, &ViewerPickHelper::coordinateTracked,
            this,     &ShapePickSession::onCoordinateTracked);
}

ShapePickSession::~ShapePickSession()
{
    // stop() clears the preview and halts the helper; no-op when already Idle.
    stop();
}

void ShapePickSession::start()
{
    if (m_state != State::Idle) {
        return;
    }

    m_collectedPoints.clear();
    m_helper->start();
    transitionTo(State::Picking);
}

void ShapePickSession::stop()
{
    if (m_state == State::Idle) {
        return;
    }

    // ViewerPickHelper::stop() internally calls clearPreview(), so no need to repeat it here.
    m_helper->stop();
    m_collectedPoints.clear();
    transitionTo(State::Idle);
}

ShapePickSession::State ShapePickSession::state() const
{
    return m_state;
}

bool ShapePickSession::isActive() const
{
    return m_state != State::Idle;
}

// -----------------------------------------------------------------------------
// Private slots
// -----------------------------------------------------------------------------

void ShapePickSession::onPointPicked(double x, double y, double z)
{
    if (m_state == State::Idle) {
        return;
    }

    m_collectedPoints.emplace_back(x, y, z);

    const int confirmedCount = static_cast<int>(m_collectedPoints.size());

    if (confirmedCount >= m_requiredPointCount) {
        // All required points collected: clear preview, emit completion signal, reset.
        m_helper->clearPreview();

        // Convert std::vector to QVector before emitting so callers need not depend on STL.
        QVector<gp_Pnt> result;
        result.reserve(confirmedCount);
        for (const auto& p : m_collectedPoints) {
            result.append(p);
        }

        // Transition to Completed first (emits stateChanged), then emit sessionCompleted.
        transitionTo(State::Completed);
        emit sessionCompleted(result);

        // Automatically return to Idle so callers can restart if needed.
        m_collectedPoints.clear();
        m_helper->stop();
        transitionTo(State::Idle);
    } else {
        // More points still needed: enter Preview state so mouse-move updates the shape.
        transitionTo(State::Preview);
    }
}

void ShapePickSession::onCoordinateTracked(double x, double y, double z)
{
    // Only update the preview when at least one point has been confirmed.
    if (m_state == State::Preview) {
        updatePreview(gp_Pnt(x, y, z));
    }
}

// -----------------------------------------------------------------------------
// Private helpers
// -----------------------------------------------------------------------------

void ShapePickSession::transitionTo(State newState)
{
    if (m_state == newState) {
        return;
    }
    m_state = newState;
    emit stateChanged(newState);
}

void ShapePickSession::updatePreview(const gp_Pnt& mousePt)
{
    if (!m_previewBuilder || m_collectedPoints.empty()) {
        return;
    }

    const TopoDS_Shape previewShape = m_previewBuilder(m_collectedPoints, mousePt);

    if (previewShape.IsNull()) {
        m_helper->clearPreview();
    } else {
        // Use a pale yellow (1.0, 1.0, 0.3) for previews to visually distinguish
        // them from the final shape.
        m_helper->setPreviewShape(previewShape, 1.0, 1.0, 0.3);
    }
}
