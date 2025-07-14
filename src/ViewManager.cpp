#include "ViewManager.h"

ViewManager &ViewManager::getInstance()
{ 
    static ViewManager instance;
    return instance;
}

void ViewManager::addView(OCCView *pView)
{
    m_view = pView;
}

OCCView* ViewManager::getActiveView()
{
    return m_view;
}

ViewManager::ViewManager(){

}