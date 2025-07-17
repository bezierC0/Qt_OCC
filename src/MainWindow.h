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
    /* file */
    void openFile();

    /* view */
    void viewFit() const;
    void onSelectModeToggled(bool checked);

    /* tool */
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
    void createLine();
    void createRectangle();
    void createCircle();
    void createArc();
    void createEllipse();
    void createPolygon();
    void createBezierCurve();
    void createNurbsCurve();
    void createBox();
    void createPyramid();
    void createSphere();
    void createCylinder();
    void createCone();

    /* help */
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

    SARibbonCategory* m_toolCategory;
    SARibbonPannel* m_transformToolPannel;
    SARibbonPannel* m_analysisPannel;
    SARibbonPannel* m_clippingPannel;
    SARibbonPannel* m_measurePannel;
    SARibbonPannel* m_otherPannel;
    QAction* m_transformAction;
    QAction* m_interferenceAction;
    QAction* m_clippingAction;
    QAction* m_explosionAction;
    QAction* m_measureDistanceAction;
    QAction* m_measureLengthAction;
    QAction* m_measureArcLengthAction;
    QAction* m_measureAngleAction;

    SARibbonCategory* m_shapeCategory;
    SARibbonPannel* m_basicShapesPannel;
    SARibbonPannel* m_2dShapesPannel;
    QAction* m_boxAction;
    QAction* m_sphereAction;
    QAction* m_cylinderAction;
    QAction* m_coneAction;

    QAction* m_lineAction;
    QAction* m_rectangleAction;
    QAction* m_circleAction;
    QAction* m_arcAction;
    QAction* m_ellipseAction;
    QAction* m_polygonAction;
    QAction* m_bezierCurveAction;
    QAction* m_nurbsCurveAction;

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
