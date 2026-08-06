#include "SelectionPickSession.h"

#include "OCCView.h"
#include "ViewManager.h"

#include <algorithm>

SelectionPickSession::SelectionPickSession(QObject* parent)
    : QObject(parent)
{
}

SelectionPickSession::~SelectionPickSession()
{
    stop();
}

bool SelectionPickSession::start(const Options& options)
{
    stop();

    OCCView* activeView = ViewManager::getInstance().getActiveView();
    if (!activeView || options.acceptedTypes.empty()) {
        return false;
    }

    m_view = activeView;
    m_savedMouseMode = static_cast<int>(activeView->getMouseMode());
    m_savedFilters = activeView->getSelectionFilters();

    if (options.clearExistingSelection) {
        activeView->clearSelectedObjects();
    }
    if (options.exclusiveFilters) {
        for (const auto& filter : m_savedFilters) {
            activeView->updateSelectionFilter(filter.first, false);
        }
    }
    for (const TopAbs_ShapeEnum type : options.acceptedTypes) {
        activeView->updateSelectionFilter(type, true);
    }
    activeView->setMouseMode(View::MouseMode::SELECTION);

    connect(activeView, &OCCView::signalSpaceSelected,
            this, &SelectionPickSession::shapePicked, Qt::UniqueConnection);
    connect(activeView, &QObject::destroyed, this, [this]() {
        m_view.clear();
        if (m_active) {
            m_active = false;
            emit activeChanged(false);
        }
    });

    m_active = true;
    emit activeChanged(true);
    return true;
}

void SelectionPickSession::stop()
{
    if (!m_active && !m_view) {
        return;
    }

    if (m_view) {
        disconnect(m_view, nullptr, this, nullptr);
        restoreViewState();
    }
    m_view.clear();

    if (m_active) {
        m_active = false;
        emit activeChanged(false);
    }
}

bool SelectionPickSession::isActive() const
{
    return m_active;
}

OCCView* SelectionPickSession::view() const
{
    return m_view.data();
}

void SelectionPickSession::restoreViewState()
{
    if (!m_view) {
        return;
    }

    m_view->setMouseMode(static_cast<View::MouseMode>(m_savedMouseMode));
    for (const auto& filter : m_savedFilters) {
        m_view->updateSelectionFilter(filter.first, filter.second);
    }
}
