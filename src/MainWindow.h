#pragma once

#include "SARibbon.h"

#include <QMenu>
#include <QAction>

class ViewerWidget;
class ModelTreeWidget;
class QTranslator;

class WidgetExplodeAssembly;

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
    void explosion();

    /* measure */
    void measureDistance() const;
    void measureLength() const;
    void measureArcLength() const;
    void measureAngle() const;
    
    /* shape */
    void createBox();
    void createSphere();
    void createCylinder();
    void createCone();
    void onSelectModeToggled(bool checked);
    void switchLanguage();
    void onSwitchTheme();

private:
    void setupUi();
    void createThemeActions();

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
    QAction* m_measureLengthAction;
    QAction* m_measureArcLengthAction;
    QAction* m_measureAngleAction;

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

    SARibbonPannel* m_themePannel;
    QMenu* m_themeMenu;
    QAction* m_lightThemeAction;
    QAction* m_darkThemeAction;

    WidgetExplodeAssembly* m_widgetExplodeAsm { nullptr } ;
};
