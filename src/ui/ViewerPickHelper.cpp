#include "ViewerPickHelper.h"
#include "ViewManager.h"
#include "OCCView.h"

#include <QEvent>
#include <QMouseEvent>

#include <AIS_InteractiveContext.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>

ViewerPickHelper::ViewerPickHelper(QObject* parent)
    : QObject(parent)
{
}

ViewerPickHelper::~ViewerPickHelper()
{
    if (m_isActive) {
        stop();
    }
}

OCCView* ViewerPickHelper::getView() const
{
    return ViewManager::getInstance().getActiveView();
}

void ViewerPickHelper::start()
{
    if (m_isActive) {
        return;
    }

    auto* view = getView();
    if (!view) {
        return;
    }
    // Snapshot the view's current mouse mode and selection filters so they can be restored faithfully when the picking session ends.
    saveState();

    // Link the mouse movement signal to track the world coordinates
    connect(view, &OCCView::signalMouseMove,
            this, &ViewerPickHelper::onMouseMove);

    // Install this object as a Qt event filter on the view widget so we can intercept mouse-button events before they reach the view's own handler.
    view->installEventFilter(this);

    m_isActive = true;
}

void ViewerPickHelper::stop()
{
    if (!m_isActive) {
        return;
    }

    auto* view = getView();
    if (view) {
        // Sever the real-time coordinate tracking connection.
        disconnect(view, &OCCView::signalMouseMove,
                   this, &ViewerPickHelper::onMouseMove);

        // Remove our event filter so mouse events flow normally again.
        view->removeEventFilter(this);
    }

    // clear any preview 
    clearPreview();

    // Put the view back into the state it was in before start() was called.
    restoreState();

    m_isActive = false;
}

bool ViewerPickHelper::isActive() const
{
    return m_isActive;
}

void ViewerPickHelper::setPreviewShape(const TopoDS_Shape& shape, double r, double g, double b)
{
    auto* view = getView();
    if (!view) {
        return;
    }

    const auto& context = view->Context();
    if (context.IsNull()) {
        return;
    }

    // Remove any previously displayed preview shape before showing the new one.
    if (!m_previewShape.IsNull()) {
        context->Erase(m_previewShape, false);
        m_previewShape.Nullify();
    }

    // A null (empty) shape is treated as a request to simply clear the preview.
    if (shape.IsNull()) {
        context->UpdateCurrentViewer();
        return;
    }

    m_previewShape = new AIS_Shape(shape);
    // apply  RGB color
    m_previewShape->SetColor(Quantity_Color(r, g, b, Quantity_TOC_RGB));
    // render on the topmost Graphic3d_ZLayerId_Topmost
    m_previewShape->SetZLayer(Graphic3d_ZLayerId_Topmost);
    // transparency 
    m_previewShape->SetTransparency(0.3);
    // add the shape to the context
    context->Display(m_previewShape, false);
    // Flush 
    context->UpdateCurrentViewer();
}

void ViewerPickHelper::clearPreview()
{
    auto* view = getView();
    if (!view || m_previewShape.IsNull()) {
        return;
    }

    const auto& context = view->Context();
    if (!context.IsNull()) {
        context->Erase(m_previewShape, false);
        context->UpdateCurrentViewer();
    }
    // Release the AIS handle so subsequent IsNull() checks work correctly.
    m_previewShape.Nullify();
}

bool ViewerPickHelper::eventFilter(QObject* watched, QEvent* event)
{
    if (!m_isActive) {
        return QObject::eventFilter(watched, event);
    }

    // Only react to the primary left mouse button.
    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            // emit the world coordinates 
            emit pointPicked(m_lastX, m_lastY, m_lastZ);
        }
    }

    return QObject::eventFilter(watched, event);
}

void ViewerPickHelper::onMouseMove(double x, double y, double z)
{
    // Cache the latest world-space position
    m_lastX = x;
    m_lastY = y;
    m_lastZ = z;

    emit coordinateTracked(x, y, z);
}

void ViewerPickHelper::saveState()
{
    auto* view = getView();
    if (!view) {
        return;
    }

    m_savedMouseMode = static_cast<int>(view->getMouseMode());
    m_savedFilters = view->getSelectionFilters();
}

void ViewerPickHelper::restoreState()
{
    auto* view = getView();
    if (!view) {
        return;
    }
    // Re-apply the mouse interaction mode that was active before start().
    view->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
    for (const auto& filter : m_savedFilters) {
        view->updateSelectionFilter(filter.first, filter.second);
    }
}
