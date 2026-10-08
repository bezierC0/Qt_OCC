#pragma once

#include <QObject>
#include <QPointer>

class OCCView;
class ViewManager : public QObject {
    Q_OBJECT

public:
    ViewManager(const ViewManager&) = delete;
    ViewManager& operator=(const ViewManager&) = delete;
    static ViewManager& getInstance() ;
    void addView(OCCView*);
    OCCView* getActiveView();

signals:
    void activeViewChanged(OCCView* view);

private:
    ViewManager();

    QPointer<OCCView> m_view;
};
