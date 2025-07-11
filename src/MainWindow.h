#pragma once

#include "SARibbon.h"

class ViewerWidget;
class ModelTreeWidget;
class QTranslator;

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
    void version();
    void explosion() const;
    void measureDistance() const;
    void createBox();
    void createSphere();
    void createCylinder();
    void createCone();
    void onSelectModeToggled(bool checked);
    void switchLanguage();

private:
    void setupUi();

private:
    ViewerWidget* m_viewerWidget;
    ModelTreeWidget* m_modelTreeWidget;
    QTranslator* m_translator;
    int m_currentLanguage;

    // UI elements
    SARibbonBar* m_ribbon = nullptr;
    SARibbonCategory* m_fileCategory;
    SARibbonPannel* m_filePannel;
    QAction* m_newAction;
    QAction* m_openAction;

    SARibbonCategory* m_viewCategory;
    SARibbonPannel* m_viewPannel;
    QAction* m_fitAction;
    QAction* m_selectAction;

    SARibbonCategory* m_analysisCategory;
    SARibbonPannel* m_analysisPannel;
    SARibbonPannel* m_clippingPannel;
    SARibbonPannel* m_measurePannel;
    QAction* m_interferenceAction;
    SARibbonPannel* m_otherPannel;
    QAction* m_clippingAction;
    QAction* m_explosionAction;
    QAction* m_measureDistanceAction;

    SARibbonCategory* m_shapeCategory;
    SARibbonPannel* m_basicShapesPannel;
    QAction* m_boxAction;
    QAction* m_sphereAction;
    QAction* m_cylinderAction;
    QAction* m_coneAction;

    SARibbonCategory* m_helpCategory;
    SARibbonPannel* m_versionPannel;
    QAction* m_versionAction;
    SARibbonPannel* m_languagePannel;
    QAction* m_languageAction;
};
