#pragma once
#include <QObject>
#include <QVector>
#include <functional>
#include <vector>

#include <gp_Pnt.hxx>
#include <TopoDS_Shape.hxx>

class ViewerPickHelper;

/**
 * @class ShapePickSession
 * @brief Generic multi-step interactive shape-picking session with a built-in state machine.
 *
 * State transitions:
 * @code
 *   Idle
 *    | start()
 *    V
 *   Picking  --pointPicked--> (more points needed) --> Preview
 *    ^                                                     |
 *    |        <-------------------------------------(pointPicked)
 *    |
 *     --------------- (all points collected) --> Completed --> emits sessionCompleted
 *
 *   Any state --stop()--> Idle
 * @endcode
 *
 */
class ShapePickSession : public QObject
{
    Q_OBJECT

public:

    enum class State
    {
        Idle,       ///< Inactive; waiting for start().
        Picking,    ///< Active; waiting for the next user click.
        Preview,    ///< At least one point confirmed; live preview is active.
        Completed   ///< All required points collected (transitions back to Idle immediately).
    };
    Q_ENUM(State)

    /**
     * @brief Type of the preview-shape builder functor.
     *
     * Called on every mouse-move event once at least one point has been confirmed.
     *
     * @param confirmedPts  Points confirmed so far (at least 1 element).
     * @param mousePt       Current mouse position in world coordinates.
     * @return              Preview TopoDS_Shape; return a null shape to clear the preview.
     */
    using PreviewBuilder = std::function<TopoDS_Shape(const std::vector<gp_Pnt>& confirmedPts,
                                                      const gp_Pnt&              mousePt)>;

    /**
     * @brief Constructs a ShapePickSession.
     *
     * @param requiredPointCount  Number of points needed to complete the session (>= 1).
     * @param previewBuilder      Functor that produces a preview shape from confirmed points
     *                            and the current mouse position.
     * @param parent              QObject parent for memory management.
     */
    explicit ShapePickSession(int            requiredPointCount,
                              PreviewBuilder previewBuilder,
                              QObject*       parent = nullptr);

    ~ShapePickSession() override;

    // Starts the picking session.
    void start();

    // Stops the picking session, clears the preview, and returns to Idle.
    void stop();

    // Returns the current session state.
    State state() const;

    // Returns true if the session is in any state other than Idle.
    bool isActive() const;

signals:
    /**
     * @brief Emitted when all required points have been collected.
     * @param points Confirmed points in input order.
     */
    void sessionCompleted(QVector<gp_Pnt> points);

    /**
     * @brief Emitted whenever the state machine transitions to a new state.
     * @param newState The state transitioned into.
     */
    void stateChanged(ShapePickSession::State newState);

private slots:
    // Connected to ViewerPickHelper::pointPicked.
    void onPointPicked(double x, double y, double z);

    // Connected to ViewerPickHelper::coordinateTracked.
    void onCoordinateTracked(double x, double y, double z);

private:
    // Transitions to a new state and emits stateChanged.
    void transitionTo(State newState);

    // Calls m_previewBuilder and forwards the result to ViewerPickHelper::setPreviewShape.
    void updatePreview(const gp_Pnt& mousePt);

    int                 m_requiredPointCount;    // Total number of points to collect.
    PreviewBuilder      m_previewBuilder;        // Externally supplied preview builder.
    ViewerPickHelper*   m_helper{nullptr};       // Underlying event bridge (owned by this object).
    State               m_state{State::Idle};    // Current state machine state.
    std::vector<gp_Pnt> m_collectedPoints;       // Confirmed points collected so far.
};
