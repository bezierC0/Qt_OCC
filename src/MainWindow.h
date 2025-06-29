#pragma once

#include "SARibbon.h"

class ViewerWidget;
class ModelTreeWidget;

class MainWindow : public SARibbonMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ViewerWidget* GetViewerWidget() const;
    ModelTreeWidget* GetModelTreeWidget() const;
private slots:
    void openFile();
    void viewFit() const;
    void checkInterference() const;
    void transform() const;
    void clipping() const;
    void explosion() const;
    void createBox();
    void createSphere();
    void createCylinder();
    void createCone();

private:
    ViewerWidget* m_viewerWidget;
    ModelTreeWidget* m_modelTreeWidget;
};
