#pragma once

#include <array>
#include <optional>

#include "SARibbon.h"

#include <QMenu>
#include <QAction>
#include <QLabel>
#include <QStatusBar>
#include <QUuid>
#include <memory>

class QTranslator;

class TopoDS_Shape;

class ViewerWidget;
class ModelTreeWidget;
class CaeTreeWidget;
class WidgetSetCoordinateSystem;
class WidgetExplodeAssembly;
class WidgetClipping;
class WidgetTransform;

namespace Cae {
class CaeController;
enum class ResultFieldType;
}

class MainWindow : public SARibbonMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    ViewerWidget* GetViewerWidget() const;
    ModelTreeWidget* GetModelTreeWidget() const;
public slots:
    void updateStatusMessage(const QString& msg, int timeout = 0);
private slots:
    /* file */
    void onNewFile();
    void onOpenFile();
    void onSaveFile();
    void onSaveAsFile();
    void onExportFile();
    void onExportDxf();
    void onExportDwg();
    void onExport3dpdf();
    void onExportPicture();
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
    void onExplosion();
    void onCreateWorkPlane();
    void onAnimation();
    void onBusbar();

    /* measure */
    void onMeasureDistance() const;
    void onMeasureLength() const;
    void onMeasureArcLength() const;
    void onMeasureAngle() const;
    void onMeasureMininumDistance() const;
    
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
    void onBooleanOperationAction();
    void onMirrorByPlane();
    void onMirrorByAxis();
    void onPatternLinear();
    void onPatternCircular();
    void onShapeToolShell();
    void onShapeToolChamfer();
    void onShapeToolFillet();
    void onShapeToolHole();

    /* CAE */
    void onCaeNewStaticStudy();
    void onCaeNewThermalStudy();
    void onCaeUseCurrentGeometry();
    void onCaeCreateNamedSelection();
    void onCaeAssignMaterial();
    void onCaeAddFixedSupport();
    void onCaeAddForce();
    void onCaeAddPressure();
    void onCaeGenerateMesh();
    void onCaeRunSolver();
    void onCaeShowDisplacement();
    void onCaeShowStress();
    void onCaeShowTemperature();
    void onCaeTreeMeshActivated(const QUuid& studyId);
    void onCaeTreeResultActivated(const QUuid& studyId, Cae::ResultFieldType fieldType);
    void onCaeRemoveBoundaryConditionRequested(const QUuid& studyId, const QString& name);
    void onCaeSetDeformationScale();
    void onCaeProbeResult();
    void onCaePickNodeToggled(bool enabled);
    void onCaeNodePicked(int nodeId);
    void onCaeSettings();

    /* help */
    void onSwitchLanguage();
    void onSwitchTheme();
    void onVersion();
    // function test 
    void onFunctionTest(); 

    void updateCoordInfo(double x, double y, double z);
    void updateShapeInfo(const QString& info);

private:
    void setupUi();
    void refreshCaeTree();
    void refreshCaeBoundaryVisualization();
    void resetCaeResultPresentation(bool preserveMesh);
    void presentCaeResult(Cae::ResultFieldType fieldType, bool reloadField = true);
    void showCaeNodeProbe(int nodeId);
    QString chooseCaeFaceTarget(const QString& title, bool* accepted);
    void createThemeActions();

    // Ribbon creation helper functions
    void createRibbon(); // Create ribbon bar
    void createFileGroup(); // Create file group
    void createViewGroup(); // Create view group
    void createToolGroup(); // Create tool group
    void createShapeGroup(); // Create shape group
    void createCaeGroup(); // Create CAE workflow group
    void createHelpGroup(); // Create help group

    enum StatusType {
        StatusCoord = 0,
        StatusShapeInfo
    };
    
    void setStatusText(StatusType type, const QString& text);

private:
    ViewerWidget* m_viewerWidget;
    ModelTreeWidget* m_modelTreeWidget;
    CaeTreeWidget* m_caeTreeWidget{nullptr};
    QTranslator* m_translator;
    int m_currentLanguage;

    // UI elements
    SARibbonBar* m_ribbon = nullptr;
    
    // ---- File Group ----
    SARibbonCategory* m_fileCategory;
    SARibbonPannel* m_filePannel;
    SARibbonPannel* m_exportPannel;
    SARibbonPannel* m_fileOthersPannel;
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_exportFileAction;
    QAction* m_exportDxfAction{};
    QAction* m_exportDwgAction{};
    QAction* m_export3dpdfAction{};
    QAction* m_exportPicAction;
    QAction* m_exitAction;

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


    // ---- Tool Group ----
    SARibbonCategory* m_toolCategory;
    SARibbonPannel* m_toolSelectPannel;
    SARibbonPannel* m_transformToolPannel;
    SARibbonPannel* m_analysisPannel;
    SARibbonPannel* m_clippingPannel;
    SARibbonPannel* m_measurePannel;
    SARibbonPannel* m_otherPannel;
    QAction* m_selectAction;
    QAction* m_selectFilterAction;
    QAction* m_transformAction;
    QAction* m_interferenceAction;
    QAction* m_clippingAction;
    QAction* m_explosionAction;
    QAction* m_measureDistanceAction;
    QAction* m_measureLengthAction;
    QAction* m_measureArcLengthAction;
    QAction* m_measureAngleAction;
    QAction* m_measureMinimumDistanceAction;
    QAction* m_createWorkPlaneAction;
    QAction* m_animationAction{};
    QAction* m_busbarAction{};

    // ---- Shape Group ----
    SARibbonCategory* m_shapeCategory;
    SARibbonPannel* m_shape3dPannel;
    SARibbonPannel* m_shape2dPannel;
    SARibbonPannel* m_shapeBooleanPannel;
    SARibbonPannel* m_mirrorPannel;
    SARibbonPannel* m_patternPannel;
    SARibbonPannel* m_shapeToolPannel{};
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
    QAction* m_booleanOperationAction;
    QAction* m_mirrorByPlaneAction;
    QAction* m_mirrorByAxisAction;
    QAction* m_patternLinearAction;
    QAction* m_patternCircularAction;
    QAction* m_shapeToolShellAction{};
    QAction* m_shapeToolChamferAction{};
    QAction* m_shapeToolFilletAction{};
    QAction* m_shapeToolHoleAction{};

    // ---- CAE Group ----
    SARibbonCategory* m_caeCategory{};
    SARibbonPannel* m_caeStudyPannel{};
    SARibbonPannel* m_caeGeometryPannel{};
    SARibbonPannel* m_caeMaterialPannel{};
    SARibbonPannel* m_caeBoundaryPannel{};
    SARibbonPannel* m_caeMeshPannel{};
    SARibbonPannel* m_caeSolvePannel{};
    SARibbonPannel* m_caeResultsPannel{};
    SARibbonPannel* m_caeSettingsPannel{};
    QAction* m_caeNewStaticAction{};
    QAction* m_caeNewThermalAction{};
    QAction* m_caeUseCurrentGeometryAction{};
    QAction* m_caeNamedSelectionAction{};
    QAction* m_caeAssignMaterialAction{};
    QAction* m_caeFixedSupportAction{};
    QAction* m_caeForceAction{};
    QAction* m_caePressureAction{};
    QAction* m_caeGenerateMeshAction{};
    QAction* m_caeRunSolverAction{};
    QAction* m_caeShowDisplacementAction{};
    QAction* m_caeShowStressAction{};
    QAction* m_caeShowTemperatureAction{};
    QAction* m_caeDeformationScaleAction{};
    QAction* m_caeProbeResultAction{};
    QAction* m_caePickNodeAction{};
    QAction* m_caeSettingsAction{};
    double m_caeDeformationScale{0.0};
    std::array<double, 3> m_caeForceComponents{100.0, 0.0, 0.0};
    double m_caePressureValue{1.0};
    double m_caeGlobalMeshSize{1.0};
    int m_caeProbeNodeId{0};
    std::optional<Cae::ResultFieldType> m_currentCaeResultField;


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
    QAction* m_functionTestAction; //For test

    WidgetExplodeAssembly*          m_widgetExplodeAsm { nullptr } ;
    WidgetSetCoordinateSystem*      m_widgetSetCoordinateSystem { nullptr };
    WidgetClipping*                 m_widgetClipping { nullptr };
    WidgetTransform*                m_widgetTransform { nullptr };

    QMap<StatusType, QLabel*> m_statusLabels;
    std::unique_ptr<Cae::CaeController> m_caeController;




};
