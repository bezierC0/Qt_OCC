#pragma once

#include <QObject>
#include <map>

#include <AIS_Shape.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Shape.hxx>
#include <Quantity_Color.hxx>

class OCCView;

/**
 * @class ViewerPickHelper
 * @brief A helper class that enables interactive 3D point picking in an OCC (OpenCASCADE) viewer.
 *
 * Typical usage:
 * @code
 *   auto* helper = new ViewerPickHelper(this);
 *   connect(helper, &ViewerPickHelper::pointPicked, this, &MyTool::onPointPicked);
 *   helper->start();
 *   // ... user clicks in the 3D view ...
 *   helper->stop();
 * @endcode
 *
 */
class ViewerPickHelper : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a ViewerPickHelper.
     * @param parent Optional QObject parent for memory management.
     */
    explicit ViewerPickHelper(QObject* parent = nullptr);

    ~ViewerPickHelper() override;

    /**
     * @brief Begins a picking session on the currently active OCCView.
     * Calling start() while already active is a no-op.
     */
    void start();

    /**
     * @brief Ends the active picking session and restores the original view state.
     * Calling stop() while not active is a no-op.
     */
    void stop();

    /**
     * @brief Returns whether a picking session is currently in progress.
     * @return true if start() has been called and stop() has not yet been called.
     */
    bool isActive() const;

    /**
     * @brief Displays or replaces a preview shape in the 3D viewer.
     *
     * @param shape  The TopoDS_Shape to preview. Pass a null shape to clear.
     * @param r      Red   component of the preview color, in range [0.0, 1.0]. Default 1.0.
     * @param g      Green component of the preview color, in range [0.0, 1.0]. Default 1.0.
     * @param b      Blue  component of the preview color, in range [0.0, 1.0]. Default 1.0.
     */
    void setPreviewShape(const TopoDS_Shape& shape, double r = 1.0, double g = 1.0, double b = 1.0);

    /**
     * @brief Removes the current preview shape from the 3D viewer and nullifies the handle.
     *
     * Does nothing if no preview shape is currently displayed or if the view is unavailable.
     */
    void clearPreview();

signals:
    /**
     * @brief Emitted on every mouse-move event while the picking session is active.
     */
    void coordinateTracked(double x, double y, double z);

    /**
     * @brief Emitted when the user clicks the left mouse button while the session is active.
     */
    void pointPicked(double x, double y, double z);

protected:
    /**
     * @brief Qt event filter override. Intercepts MouseButtonPress events on the watched view.
     *
     * @param watched  The QObject that received the event (expected to be the OCCView widget).
     * @param event    The Qt event to inspect.
     * @return Always returns the result of QObject::eventFilter(), so events are not blocked.
     */
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    /**
     * @brief Slot connected to OCCView::signalMouseMove. Caches the current world coordinates
     *        and re-emits them via coordinateTracked().
     *
     * @param x  World X coordinate reported by the view.
     * @param y  World Y coordinate reported by the view.
     * @param z  World Z coordinate reported by the view.
     */
    void onMouseMove(double x, double y, double z);

private:
    /**
     * @brief Saves the active view's current mouse mode and selection filters so they
     *        can be restored after the picking session ends.
     */
    void saveState();

    /**
     * @brief Restores the mouse mode and selection filters that were saved by saveState().
     *
     * Called automatically by stop(). Does nothing if no active view is available.
     */
    void restoreState();

    /**
     * @brief Convenience accessor that returns the currently active OCCView from ViewManager.
     * @return Pointer to the active OCCView, or nullptr if none exists.
     */
    OCCView* getView() const;

    bool   m_isActive{false};// Whether a picking session is currently running.
    double m_lastX{0.0};// Last X coordinate 
    double m_lastY{0.0};// Last Y coordinate 
    double m_lastZ{0.0};// Last Z coordinate 
    Handle(AIS_Shape) m_previewShape;// AIS shape handle for the optional live preview geometry displayed in the viewer.
    int m_savedMouseMode{0};// Mouse mode integer saved before the session started; restored in restoreState().
    std::map<TopAbs_ShapeEnum, bool> m_savedFilters{};// Per-shape-type selection filter states saved before the session; restored in restoreState().
};