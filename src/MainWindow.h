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
    void onNewFile();
    void onOpenFile();
    void onSaveFile();
    void onSaveAsFile();
    void onExportFile();
    void onExit();

    /* view */
    void onViewFit() const;
    void onChangeViewIsometric() const;
    void onChangeViewTop() const;
    void onChangeViewBottom() const;
    void onChangeViewLeft() const;
    void onChangeViewRight() const;
    void onChangeViewFront() const;
    void onChangeViewBack() const;
    void onSetDisplayMode(int mode) const;

    /* tool */
    void onSwitchSelect(bool checked);
    void onFilterStateChanged(int filterType, bool isChecked);
    void onCheckInterference() const;
    void onTransform() const;
    void onClipping() const;
    void onVersion();
    void onExplosion();

    /* measure */
    void onMeasureDistance() const;
    void onMeasureLength() const;
    void onMeasureArcLength() const;
    void onMeasureAngle() const;
    
    /* shape */
    void onCreatePoint();
    void onCreateLine();
    void onCreateRectangle();
    void onCreateCircle();
    void onCreateArc();
    void onCreateEllipse();
    void onCreatePolygon();
    void onCreateBezierCurve();
    void onCreateNurbsCurve();
    void onCreateBox();
    void onCreatePyramid();
    void onCreateSphere();
    void onCreateCylinder();
    void onCreateCone();
    void onBooleanUnionAction();
    void onBooleanIntersection();
    void onBooleanDifference();
    void onMirrorByPlane();
    void onMirrorByAxis();
    void onPatternLinear();
    void onPatternCircular();

    /* help */
    void onSwitchLanguage();
    void onSwitchTheme();

private:
    void setupUi();
    void createThemeActions();

    // Ribbon creation helper functions
    void createRibbon(); // Create ribbon bar
    void createFileGroup(); // Create file group
    void createViewGroup(); // Create view group
    void createToolGroup(); // Create tool group
    void createShapeGroup(); // Create shape group
    void createHelpGroup(); // Create help group

private:
    ViewerWidget* m_viewerWidget;
    ModelTreeWidget* m_modelTreeWidget;
    QTranslator* m_translator;
    int m_currentLanguage;

    // UI elements
    SARibbonBar* m_ribbon = nullptr;
    
    // ---- File Group ----
    SARibbonCategory* m_fileCategory;
    SARibbonPannel* m_filePannel;
    QAction* m_newAction;
    QAction* m_openAction;

    // ---- View Group ----
    SARibbonCategory* m_viewCategory;
    SARibbonPannel* m_viewPannel;
    SARibbonPannel* m_viewChangePannel;
    SARibbonPannel* m_displayModePannel;
    QAction* m_fitAction;
    QAction* m_viewIsometricAction;
    QAction* m_viewTopAction;
    QAction* m_viewBottomAction;
    QAction* m_viewLeftAction;
    QAction* m_viewRightAction;
    QAction* m_viewFrontAction;
    QAction* m_viewBackAction;
    QAction* m_displayModeAction;
    QAction* m_selectAction;
    QAction* m_selectFilterAction;

    // ---- Tool Group ----
    SARibbonCategory* m_toolCategory;
    SARibbonPannel* m_toolSelectPannel;
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

    // ---- Shape Group ----
    SARibbonCategory* m_shapeCategory;
    SARibbonPannel* m_shape3dPannel;
    SARibbonPannel* m_shape2dPannel;
    SARibbonPannel* m_shapeBooleanPannel;
    SARibbonPannel* m_mirrorPannel;
    SARibbonPannel* m_patternPannel;
    QAction* m_boxAction;
    QAction* m_sphereAction;
    QAction* m_cylinderAction;
    QAction* m_coneAction;

    QAction* m_pointAction;
    QAction* m_lineAction;
    QAction* m_rectangleAction;
    QAction* m_circleAction;
    QAction* m_arcAction;
    QAction* m_ellipseAction;
    QAction* m_polygonAction;
    QAction* m_bezierCurveAction;
    QAction* m_nurbsCurveAction;
    QAction* m_booleanUnionAction;
    QAction* m_booleanIntersectionAction;
    QAction* m_booleanDifferenceAction;
    QAction* m_mirrorByPlaneAction;
    QAction* m_mirrorByAxisAction;
    QAction* m_patternLinearAction;
    QAction* m_patternCircularAction;


    // ---- help Group ----
    SARibbonCategory* m_helpCategory;
    SARibbonPannel* m_versionPannel;
    SARibbonPannel* m_languagePannel;
    SARibbonPannel* m_themePannel;
    QMenu* m_themeMenu;
    QAction* m_versionAction;
    QAction* m_languageAction;
    QAction* m_lightThemeAction;
    QAction* m_darkThemeAction;

    WidgetExplodeAssembly* m_widgetExplodeAsm { nullptr } ;
};
