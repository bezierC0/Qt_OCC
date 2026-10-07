#include "ViewManager.h"
#include "OCCView.h"

ViewManager &ViewManager::getInstance()
{ 
    static ViewManager instance;
    return instance;
}

void ViewManager::addView(OCCView *pView)
{
    if (m_view == pView) return;

    if (m_view) {
        disconnect(m_view, nullptr, this, nullptr);
    }
    m_view = pView;
    if (m_view) {
        connect(m_view, &QObject::destroyed, this, [this]() {
            m_view.clear();
            emit activeViewChanged(nullptr);
        });
    }
    emit activeViewChanged(m_view.data());
}

OCCView* ViewManager::getActiveView()
{
    return m_view;
}

ViewManager::ViewManager()
    : QObject(nullptr)
{

}
