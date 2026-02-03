
class OCCView;
class ViewManager {
public:
    ViewManager(const ViewManager&) = delete;
    ViewManager& operator=(const ViewManager&) = delete;
    static ViewManager& getInstance() ;
    void addView(OCCView*);
    OCCView* getActiveView();
private:
    ViewManager();

    OCCView* m_view;
};