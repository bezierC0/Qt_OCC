#include "MainWindow.h"
#include <QtCore>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QAction>
#include <QCoreApplication>
#include <QTranslator>
#include <QEvent>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QToolButton>
#include <QStatusBar>
#include <QLabel>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QTabWidget>
#include <algorithm>
#include <utility>

// Note: TopoDS_Shape and TopAbs_ShapeEnum are available through ViewerWidget.h includes

#include "ViewerWidget.h"
#include "WidgetModelTree.h"
#include "WidgetCaeTree.h"
#include "DialogAbout.h"
#include "DialogCaeForce.h"
#include "DialogCaeMaterial.h"
#include "DialogCaeSettings.h"
#include "widget_explode_assembly.h"
#include "widget_clipping.h"
#include "widget_set_coordinate_system.h"
#include "widget_transform.h"
#include "cae/CaeBoundaryVisualization.h"
#include "cae/CaeCommand.h"
#include "cae/CaeController.h"
#include "cae/CaeExternalToolConfig.h"
#include "cae/CaeExternalToolConfigStore.h"

MainWindow::MainWindow(QWidget* parent) : SARibbonMainWindow(parent)
{
    setWindowTitle("QT_OOC");
    setWindowIcon(QIcon(":/images/images/logo.png"));
    m_translator = new QTranslator(this);
    m_currentLanguage = 0; // 0: English, 1: Chinese, 2: Japanese

    m_modelTreeWidget = new ModelTreeWidget( this );
    m_caeTreeWidget = new CaeTreeWidget(this);
    m_viewerWidget = new ViewerWidget(this);
    m_caeController = std::make_unique<Cae::CaeController>();
    m_caeController->setExternalToolConfig(Cae::CaeExternalToolConfigStore().load());

    auto* navigationTabs = new QTabWidget(this);
    navigationTabs->addTab(m_modelTreeWidget, tr("Model"));
    navigationTabs->addTab(m_caeTreeWidget, tr("CAE"));

    const auto splitter = new QSplitter( Qt::Horizontal, this );
    splitter->addWidget( navigationTabs );
    splitter->addWidget( m_viewerWidget );

    splitter->setStretchFactor( 0, 1 ); // m_modelTreeWidget
    splitter->setStretchFactor( 1, 4 ); 

    connect(m_modelTreeWidget, &ModelTreeWidget::labelSelected,      m_viewerWidget, &ViewerWidget::highlightLabel);
    connect(m_modelTreeWidget, &ModelTreeWidget::labelPickRequested, m_viewerWidget, &ViewerWidget::highlightLabel);
    connect(m_modelTreeWidget, &ModelTreeWidget::labelRemoveRequested, m_viewerWidget, &ViewerWidget::removeLabelShape);
    connect(m_viewerWidget, &ViewerWidget::signalCaeNodePicked, this, &MainWindow::onCaeNodePicked);
    connect(m_caeTreeWidget, &CaeTreeWidget::meshActivated, this, &MainWindow::onCaeTreeMeshActivated);
    connect(m_caeTreeWidget, &CaeTreeWidget::resultFieldActivated, this, &MainWindow::onCaeTreeResultActivated);
    connect(
        m_caeTreeWidget,
        &CaeTreeWidget::removeBoundaryConditionRequested,
        this,
        &MainWindow::onCaeRemoveBoundaryConditionRequested);

    setCentralWidget( splitter );
    refreshCaeTree();

    setupUi();

    resize(1200, 800);
}

MainWindow::~MainWindow() = default;

void MainWindow::refreshCaeTree()
{
    if (m_caeTreeWidget && m_caeController) {
        m_caeTreeWidget->setProject(m_caeController->project());
    }
}

void MainWindow::refreshCaeBoundaryVisualization()
{
    Cae::BoundaryMarkers markers;
    const Cae::CaeStudy* study = m_caeController->project().activeStudy();
    if (study) {
        for (const Cae::CaeBoundaryCondition& condition : study->boundaryConditions()) {
            const Cae::CaeNamedSelection* target = study->findNamedSelection(condition.targetName());
            if (!target || !target->planarRegion()) {
                continue;
            }

            Cae::BoundaryMarker marker;
            marker.label = condition.name();
            marker.region = *target->planarRegion();
            switch (condition.type()) {
            case Cae::BoundaryConditionType::FixedSupport:
                marker.type = Cae::BoundaryMarkerType::FixedSupport;
                break;
            case Cae::BoundaryConditionType::Force:
                marker.type = Cae::BoundaryMarkerType::Force;
                marker.direction = condition.components();
                break;
            case Cae::BoundaryConditionType::Pressure:
                marker.type = Cae::BoundaryMarkerType::Pressure;
                marker.direction = {
                    -marker.region.normal[0],
                    -marker.region.normal[1],
                    -marker.region.normal[2]};
                break;
            }
            markers.push_back(std::move(marker));
        }
    }
    m_viewerWidget->showCaeBoundaryMarkers(markers);
}

void MainWindow::setupUi()
{
    createRibbon();
    createFileGroup();
    createViewGroup();
    createToolGroup();
    createShapeGroup();
    createCaeGroup();
    createHelpGroup();

    m_widgetExplodeAsm = new WidgetExplodeAssembly();
    createThemeActions();

    m_widgetSetCoordinateSystem = new WidgetSetCoordinateSystem(this);
    m_widgetClipping = new WidgetClipping(this);
    m_widgetTransform = new WidgetTransform(this);

    // Status Bar Setup
    QStatusBar* statusBar = this->statusBar();
    
    auto createStatusLabel = [&](StatusType type, const QString& defaultText, int minWidth) {
        QLabel* label = new QLabel(defaultText, this);
        label->setMinimumWidth(minWidth);
        statusBar->addPermanentWidget(label);
        m_statusLabels[type] = label;
    };

    createStatusLabel(StatusCoord, tr("X: 0.00 Y: 0.00 Z: 0.00"), 150);
    createStatusLabel(StatusShapeInfo, tr("Selected: None"), 150);

    // Connect signals
    connect(m_viewerWidget, &ViewerWidget::signalMouseMove, this, &MainWindow::updateCoordInfo);
    connect(m_viewerWidget, &ViewerWidget::signalSelectionInfo, this, &MainWindow::updateShapeInfo);
}

void MainWindow::createRibbon() {
    if ( m_ribbon )
    {
        m_ribbon->clear();
        m_ribbon->removeCategory( m_fileCategory );
        m_ribbon->removeCategory( m_viewCategory );
        m_ribbon->removeCategory( m_toolCategory );
        m_ribbon->removeCategory( m_shapeCategory );
        m_ribbon->removeCategory( m_caeCategory );
        m_ribbon->removeCategory( m_helpCategory );
    }
    else
    {
        m_ribbon = new SARibbonBar(this);
        setRibbonBar( m_ribbon );
    }
    
    m_ribbon->setRibbonStyle(SARibbonBar::RibbonStyleLooseThreeRow); // Or other styles like WpsLiteStyle
}

void MainWindow::createFileGroup()
{
    // ---- File Group ----
    m_fileCategory = m_ribbon->addCategoryPage(tr("File"));
    m_filePannel = m_fileCategory->addPannel(tr("File Operations"));

    // new
    m_newAction = new QAction(QIcon(":/icons/icon/file_new.svg"), tr("New"), this); // Assuming an icon path
    connect(m_newAction, &QAction::triggered, this, &MainWindow::onNewFile);
    m_filePannel->addLargeAction(m_newAction);

    // open
    m_openAction = new QAction(QIcon(":/icons/icon/open.png"), tr("Open"), this); // Assuming an icon path
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    m_filePannel->addLargeAction(m_openAction);

    // export panel
    auto createExportPannel = [&](){
        m_exportPannel = m_fileCategory->addPannel(tr("Export"));

        m_exportFileAction = new QAction(QIcon(":/icons/icon/file_export_file.svg"), tr("File"), this); 
        connect(m_exportFileAction, &QAction::triggered, this, &MainWindow::onExportFile);
        m_exportPannel->addLargeAction(m_exportFileAction);

        m_exportDxfAction = new QAction(QIcon(":/icons/icon/file_export_dxf.svg"), tr("DXF"), this); 
        connect(m_exportDxfAction, &QAction::triggered, this, &MainWindow::onExportDxf);
        m_exportPannel->addLargeAction(m_exportDxfAction);

        m_exportDwgAction = new QAction(QIcon(":/icons/icon/file_export_dwg.svg"), tr("DWG"), this); 
        connect(m_exportDwgAction, &QAction::triggered, this, &MainWindow::onExportDwg);
        m_exportPannel->addLargeAction(m_exportDwgAction);

        m_export3dpdfAction = new QAction(QIcon(":/icons/icon/file_export_3dpdf.svg"), tr("3D PDF"), this); 
        connect(m_export3dpdfAction, &QAction::triggered, this, &MainWindow::onExport3dpdf);
        m_exportPannel->addLargeAction(m_export3dpdfAction);

        m_exportPicAction = new QAction(QIcon(":/icons/icon/file_export_picture.svg"), tr("Picture"), this); 
        connect(m_exportPicAction, &QAction::triggered, this, &MainWindow::onExportPicture);
        m_exportPannel->addLargeAction(m_exportPicAction);
    };

    createExportPannel();

    // ---- File Others Group ----
    m_fileOthersPannel = m_fileCategory->addPannel(tr("Others"));

    // exit
    m_exitAction = new QAction(QIcon(":/icons/icon/file_exit.svg"), tr("Exit"), this); // Assuming an icon path
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::onExit);
    m_fileOthersPannel->addLargeAction(m_exitAction);
}

void MainWindow::createViewGroup()
{
    // ---- View Group ----
    m_viewCategory = m_ribbon->addCategoryPage(tr("View"));

    // view panel
    auto createViewPannel = [&](){
        m_viewPannel = m_viewCategory->addPannel(tr("View Operations"));

        m_fitAction = new QAction(QIcon(":/icons/icon/fit.png"), tr("Fit"), this); // Assuming an icon path
        connect(m_fitAction, &QAction::triggered, this, &MainWindow::onViewFit);
        m_viewPannel->addLargeAction(m_fitAction);
    };
    createViewPannel();

    // view change
    auto createViewChangePannel = [&](){
        m_viewChangePannel = m_viewCategory->addPannel(tr("View Change"));
        // isometric
        m_viewIsometricAction = new QAction(QIcon(":/icons/icon/view_isometric.svg"), tr("Isometric"), this);
        connect(m_viewIsometricAction, &QAction::triggered, this, &MainWindow::onChangeViewIsometric);
        m_viewChangePannel->addSmallAction(m_viewIsometricAction);

        // top
        m_viewTopAction = new QAction(QIcon(":/icons/icon/view_top.svg"), tr("Top"), this);
        connect(m_viewTopAction, &QAction::triggered, this, &MainWindow::onChangeViewTop);
        m_viewChangePannel->addSmallAction(m_viewTopAction);

        // bottom
        m_viewBottomAction = new QAction(QIcon(":/icons/icon/view_bottom.svg"), tr("Bottom"), this);
        connect(m_viewBottomAction, &QAction::triggered, this, &MainWindow::onChangeViewBottom);
        m_viewChangePannel->addSmallAction(m_viewBottomAction);

        // left
        m_viewLeftAction = new QAction(QIcon(":/icons/icon/view_left.svg"), tr("Left"), this);
        connect(m_viewLeftAction, &QAction::triggered, this, &MainWindow::onChangeViewLeft);
        m_viewChangePannel->addSmallAction(m_viewLeftAction);

        // right
        m_viewRightAction = new QAction(QIcon(":/icons/icon/view_right.svg"), tr("Right"), this);
        connect(m_viewRightAction, &QAction::triggered, this, &MainWindow::onChangeViewRight);
        m_viewChangePannel->addSmallAction(m_viewRightAction);

        // front
        m_viewFrontAction = new QAction(QIcon(":/icons/icon/view_front.svg"), tr("Front"), this);
        connect(m_viewFrontAction, &QAction::triggered, this, &MainWindow::onChangeViewFront);
        m_viewChangePannel->addSmallAction(m_viewFrontAction);

        // back
        m_viewBackAction = new QAction(QIcon(":/icons/icon/view_back.svg"), tr("Back"), this);
        connect(m_viewBackAction, &QAction::triggered, this, &MainWindow::onChangeViewBack);
        m_viewChangePannel->addSmallAction(m_viewBackAction);
    };
    createViewChangePannel();

    // display Mode Pannel
    auto createViewDisplayModePannel = [&](){
        m_displayModePannel = m_viewCategory->addPannel(tr("Display Mode"));
        // display mode
        m_displayModeAction = new QAction(QIcon(":/icons/icon/display_mode.svg"), tr("Style"), this);
        m_displayModePannel->addSmallAction(m_displayModeAction);

        SARibbonMenu* menu = new SARibbonMenu(this);
        m_displayModeAction->setMenu(menu);
        m_displayModeAction->setToolTip(tr("Display Mode"));
        menu->setStyleSheet("QMenu::item { padding-left: 25px; }");

        QActionGroup* displayModeGroup = new QActionGroup(this);
        displayModeGroup->setExclusive(true);

        auto createDisplayAction = [&](const QIcon& icon,const QString& text, int mode) {
            //auto action = new QAction(icon,text, this);
            auto action = new QAction(text, this);
            action->setIcon(icon);
            action->setCheckable(true);
            connect(action, &QAction::triggered, this, [this, mode]() {
                m_viewerWidget->setDisplayMode(mode);
            });
            displayModeGroup->addAction(action);
            menu->addAction(action);
            return action;
        };

        auto shadingAction = createDisplayAction(QIcon(":/icons/icon/display_mode_shading.svg"),tr("Shading"), 0);
        auto wireframeAction = createDisplayAction(QIcon(":/icons/icon/display_mode_wireframe.svg"),tr("Wireframe"), 1);
        auto hiddenLineAction = createDisplayAction(QIcon(":/icons/icon/display_mode_hidden_line.svg"),tr("Hidden Line"), 2);
        auto shadingEdgeAction = createDisplayAction(QIcon(":/icons/icon/display_mode_shading_edge.svg"),tr("Shading with edge"), 3);

        shadingAction->setChecked(true); // Default selection
    };
    createViewDisplayModePannel();

}

void MainWindow::createToolGroup()
{
    // ---- Tool Group ----
    m_toolCategory = m_ribbon->addCategoryPage(tr("Tool"));

    /* Create select panel*/
    auto createToolSelectPannel = [&](){
        m_toolSelectPannel = m_toolCategory->addPannel(tr("Select Operations"));
        // select
        m_selectAction = new QAction(QIcon(":/icons/icon/select.svg"), tr("Select"), this);
        m_selectAction->setCheckable(true);
        connect(m_selectAction, &QAction::toggled, this, &MainWindow::onSwitchSelect);
        m_toolSelectPannel->addLargeAction(m_selectAction);

        // Select Filter
        auto createAction = [&](const QString& text, const QString& iconurl)
        {
            QAction *act = new QAction(this);
            act->setText(text);
            act->setIcon(QIcon(iconurl));
            act->setObjectName(text);
            return act;
        };
        m_selectFilterAction = createAction(tr("Select Filter"), ":/icons/icon/select_filter.svg");

        SARibbonMenu* filterMenu = new SARibbonMenu(this);
        m_selectFilterAction->setMenu(filterMenu);

        // Create checkboxes for each filter type
        QCheckBox* vertexCheckBox = new QCheckBox(tr("Vertex"));
        QCheckBox* edgeCheckBox = new QCheckBox(tr("Edge"));
        QCheckBox* faceCheckBox = new QCheckBox(tr("Face"));
        QCheckBox* solidCheckBox = new QCheckBox(tr("Solid"));

        // Set initial checkbox states based on current filters
        const auto& currentFilters = m_viewerWidget->getSelectionFilters();
        vertexCheckBox->setChecked(currentFilters.at(TopAbs_VERTEX));
        edgeCheckBox->setChecked(currentFilters.at(TopAbs_EDGE));
        faceCheckBox->setChecked(currentFilters.at(TopAbs_FACE));
        solidCheckBox->setChecked(currentFilters.at(TopAbs_SOLID));

        connect(vertexCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
            onFilterStateChanged(TopAbs_VERTEX, state == Qt::Checked);
        });
        connect(edgeCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
            onFilterStateChanged(TopAbs_EDGE, state == Qt::Checked);
        });
        connect(faceCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
            onFilterStateChanged(TopAbs_FACE, state == Qt::Checked);
        });
        connect(solidCheckBox, &QCheckBox::stateChanged, this, [this](int state) {
            onFilterStateChanged(TopAbs_SOLID, state == Qt::Checked);
        });

        filterMenu->addWidget(vertexCheckBox);
        filterMenu->addWidget(edgeCheckBox);
        filterMenu->addWidget(faceCheckBox);
        filterMenu->addWidget(solidCheckBox);

        m_toolSelectPannel->addAction(m_selectFilterAction);
    };
    createToolSelectPannel();

    /* Create Transform Pannel*/
    auto createTransformToolPannel = [&](){
        /* Transform Pannel*/
        m_transformToolPannel = m_toolCategory->addPannel(tr("Transform Tool"));
        // analysisPannel interference
        m_transformAction = new QAction(QIcon(":/icons/icon/tool_transform.svg"), tr("Transform"), this); // Assuming an icon path
        connect(m_transformAction, &QAction::triggered, this, &MainWindow::onTransform);
        m_transformToolPannel->addLargeAction(m_transformAction);
    };
    createTransformToolPannel();

    /* Create Analysis Pannel*/
    auto createAnalysisPannel = [&](){
        /* Analysis Pannel*/
        m_analysisPannel = m_toolCategory->addPannel(tr("Analysis"));
        // analysisPannel interference
        m_interferenceAction = new QAction(QIcon(":/icons/icon/interference.png"), tr("Interference"), this); // Assuming an icon path
        connect(m_interferenceAction, &QAction::triggered, this, &MainWindow::onCheckInterference);
        m_analysisPannel->addLargeAction(m_interferenceAction);
    };
    createAnalysisPannel();

    /* Create Clipping Pannel*/
    auto createClippingPannel = [&](){
        /* Clipping Pannel */
        m_clippingPannel = m_toolCategory->addPannel(tr("Clipping Tool"));
        // clipping
        m_clippingAction = new QAction(QIcon(":/icons/icon/clipping.svg"), tr("Clipping"), this); // Assuming an icon path
        connect(m_clippingAction, &QAction::triggered, this, &MainWindow::onClipping);
        m_clippingPannel->addLargeAction(m_clippingAction);
    };
    
    createClippingPannel();
    /* create Measure Pannel */
    auto createMeasurePannel = [&](){
        /* Measure Pannel */
        m_measurePannel = m_toolCategory->addPannel(tr("Measure"));
        // measure distance
        m_measureDistanceAction = new QAction(QIcon(":/icons/icon/measure_distance.svg"), tr("Distance"), this);
        connect(m_measureDistanceAction, &QAction::triggered, this, &MainWindow::onMeasureDistance);
        m_measurePannel->addSmallAction(m_measureDistanceAction);

        // measure length
        m_measureLengthAction = new QAction(QIcon(":/icons/icon/measure_length.svg"), tr("Length"), this);
        connect(m_measureLengthAction, &QAction::triggered, this, &MainWindow::onMeasureLength);
        m_measurePannel->addSmallAction(m_measureLengthAction);

        // measure arc length
        m_measureArcLengthAction = new QAction(QIcon(":/icons/icon/measure_arc_length.svg"), tr("Arc"), this);
        connect(m_measureArcLengthAction, &QAction::triggered, this, &MainWindow::onMeasureArcLength);
        m_measurePannel->addSmallAction(m_measureArcLengthAction);

        // measure angle
        m_measureAngleAction = new QAction(QIcon(":/icons/icon/measure_angle.svg"), tr("Angle"), this); 
        connect(m_measureAngleAction, &QAction::triggered, this, &MainWindow::onMeasureAngle);
        m_measurePannel->addSmallAction(m_measureAngleAction);

        // measure minimum distance
        m_measureMinimumDistanceAction = new QAction(QIcon(":/icons/icon/measure_length.svg"), tr("Minimum Distance"), this); 
        connect(m_measureMinimumDistanceAction, &QAction::triggered, this, &MainWindow::onMeasureMininumDistance);
        m_measurePannel->addSmallAction(m_measureMinimumDistanceAction);
    };
    createMeasurePannel();

    /* Create Other Pannel */
    auto createOtherPannel = [&](){
        /* Other Pannel */
        m_otherPannel = m_toolCategory->addPannel(tr("Other Tools")); // Adding to Analysis category for simplicity
        // explosion
        m_explosionAction = new QAction(QIcon(":/icons/icon/tool_explosion.svg"), tr("Explosion"), this); // Assuming an icon path
        connect(m_explosionAction, &QAction::triggered, this, &MainWindow::onExplosion);
        m_otherPannel->addLargeAction(m_explosionAction);

        // createWorkPlaneAction
        m_createWorkPlaneAction = new QAction(QIcon(":/icons/icon/work_plane.svg"), tr("Work Plane"), this);
        connect(m_createWorkPlaneAction, &QAction::triggered, this, &MainWindow::onCreateWorkPlane);
        m_otherPannel->addLargeAction(m_createWorkPlaneAction);

        // animation
        m_animationAction = new QAction(QIcon(":/icons/icon/animation.svg"), tr("Animation"), this);
        connect(m_animationAction, &QAction::triggered, this, &MainWindow::onAnimation);
        m_otherPannel->addLargeAction(m_animationAction);

        // busbar
        m_busbarAction = new QAction(QIcon(":/icons/icon/busbar.svg"), tr("Busbar"), this);
        connect(m_busbarAction, &QAction::triggered, this, &MainWindow::onBusbar);
        m_otherPannel->addLargeAction(m_busbarAction);
    };
    createOtherPannel();

}

void MainWindow::createShapeGroup()
{
    // ---- Shape Group ----
    m_shapeCategory = m_ribbon->addCategoryPage(tr("Shape"));
    
    // ---- 2d Shape Pannel ----
    auto createShape2dPannel = [&](){
        m_shape2dPannel = m_shapeCategory->addPannel(tr("2D"));

        // point
        m_pointAction = new QAction(QIcon(":/icons/icon/shape_point.svg"),tr("Point"), this);
        connect(m_pointAction, &QAction::triggered, this, &MainWindow::onCreatePoint);
        m_shape2dPannel->addSmallAction(m_pointAction);

        // line
        m_lineAction = new QAction(QIcon(":/icons/icon/shape_line.svg"),tr("Line"), this);
        connect(m_lineAction, &QAction::triggered, this, &MainWindow::onCreateLine);
        m_shape2dPannel->addSmallAction(m_lineAction);

        // rectangle
        m_rectangleAction = new QAction(QIcon(":/icons/icon/shape_rectangle.svg"),tr("Rectangle"), this);
        connect(m_rectangleAction, &QAction::triggered, this, &MainWindow::onCreateRectangle);
        m_shape2dPannel->addSmallAction(m_rectangleAction);

        // circle
        m_circleAction = new QAction(QIcon(":/icons/icon/shape_circle.svg"),tr("Circle"), this);
        connect(m_circleAction, &QAction::triggered, this, &MainWindow::onCreateCircle);
        m_shape2dPannel->addSmallAction(m_circleAction);

        // arc
        m_arcAction = new QAction(QIcon(":/icons/icon/shape_arc.svg"),tr("Arc"), this);
        connect(m_arcAction, &QAction::triggered, this, &MainWindow::onCreateArc);
        m_shape2dPannel->addSmallAction(m_arcAction);

        // ellipse
        m_ellipseAction = new QAction(QIcon(":/icons/icon/shape_ellipse.svg"),tr("Ellipse"), this);
        connect(m_ellipseAction, &QAction::triggered, this, &MainWindow::onCreateEllipse);
        m_shape2dPannel->addSmallAction(m_ellipseAction);

        // polygon
        m_polygonAction = new QAction(QIcon(":/icons/icon/shape_polyline.svg"),tr("Polygon"), this);
        connect(m_polygonAction, &QAction::triggered, this, &MainWindow::onCreatePolygon);
        m_shape2dPannel->addSmallAction(m_polygonAction);

        // bezier
        m_bezierCurveAction = new QAction(QIcon(":/icons/icon/shape_bezier.svg"),tr("Bezier"), this);
        connect(m_bezierCurveAction, &QAction::triggered, this, &MainWindow::onCreateBezierCurve);
        m_shape2dPannel->addSmallAction(m_bezierCurveAction);

        // nurbs
        m_nurbsCurveAction = new QAction(QIcon(":/icons/icon/shape_nurbs.svg"),tr("Nurbs"), this);
        connect(m_nurbsCurveAction, &QAction::triggered, this, &MainWindow::onCreateNurbsCurve);
        m_shape2dPannel->addSmallAction(m_nurbsCurveAction);
    };
    createShape2dPannel();

    auto createShape3dPannel = [&](){
        // ---- 3D Shape Pannel ----
        m_shape3dPannel = m_shapeCategory->addPannel(tr("3D"));

        // box
        m_boxAction = new QAction(QIcon(":/icons/icon/box.png"), tr("Box"), this);
        connect(m_boxAction, &QAction::triggered, this, &MainWindow::onCreateBox);
        m_shape3dPannel->addLargeAction(m_boxAction);

        // sphere
        m_sphereAction = new QAction(QIcon(":/icons/icon/sphere.png"), tr("Sphere"), this);
        connect(m_sphereAction, &QAction::triggered, this, &MainWindow::onCreateSphere);
        m_shape3dPannel->addLargeAction(m_sphereAction);

        // cylinder
        m_cylinderAction = new QAction(QIcon(":/icons/icon/cylinder.png"), tr("Cylinder"), this);
        connect(m_cylinderAction, &QAction::triggered, this, &MainWindow::onCreateCylinder);
        m_shape3dPannel->addLargeAction(m_cylinderAction);

        // cone
        m_coneAction = new QAction(QIcon(":/icons/icon/cone.png"), tr("Cone"), this);
        connect(m_coneAction, &QAction::triggered, this, &MainWindow::onCreateCone);
        m_shape3dPannel->addLargeAction(m_coneAction);
    };
    createShape3dPannel();

    /* shape Boolean Pannel Pannel  */
    auto createShapeBooleanPannel = [&](){
        m_shapeBooleanPannel = m_shapeCategory->addPannel(tr("Boolean"));

        // boolean operation
        m_booleanOperationAction = new QAction(QIcon(":/icons/icon/boolean_intersection.svg"), tr("Boolean"), this);
        connect(m_booleanOperationAction, &QAction::triggered, this, &MainWindow::onBooleanOperationAction);
        m_shapeBooleanPannel->addLargeAction(m_booleanOperationAction);
    };
    createShapeBooleanPannel();

    /* mirror panel */
    auto createMirrorPannel = [&](){
        m_mirrorPannel = m_shapeCategory->addPannel(tr("Mirror"));

        m_mirrorByPlaneAction = new QAction(QIcon(":/icons/icon/mirror_plane.svg"), tr("Mirror Plane"), this);
        connect(m_mirrorByPlaneAction, &QAction::triggered, this, &MainWindow::onMirrorByPlane);
        m_mirrorPannel->addLargeAction(m_mirrorByPlaneAction);

        m_mirrorByAxisAction = new QAction(QIcon(":/icons/icon/mirror_axis.svg"), tr("Mirror Axis"), this);
        connect(m_mirrorByAxisAction, &QAction::triggered, this, &MainWindow::onMirrorByAxis);
        m_mirrorPannel->addLargeAction(m_mirrorByAxisAction);
    };
    createMirrorPannel();

    /* pattern panel */
    auto createPatternPannel = [&](){
        m_patternPannel = m_shapeCategory->addPannel(tr("Pattern"));

        m_patternLinearAction = new QAction(QIcon(":/icons/icon/patter_linear.svg"), tr("Linear Pattern"), this);
        connect(m_patternLinearAction, &QAction::triggered, this, &MainWindow::onPatternLinear);
        m_patternPannel->addLargeAction(m_patternLinearAction);

        m_patternCircularAction = new QAction(QIcon(":/icons/icon/pattern_circular.svg"), tr("Circular Pattern"), this);
        connect(m_patternCircularAction, &QAction::triggered, this, &MainWindow::onPatternCircular);
        m_patternPannel->addLargeAction(m_patternCircularAction);
    };
    createPatternPannel();

    /*   shape tool pannel   */
    auto createShapeToolPannel = [&](){
        m_shapeToolPannel = m_shapeCategory->addPannel(tr("Shape Tool"));

        m_shapeToolShellAction = new QAction(QIcon(":/icons/icon/shape_tool_shell.svg"), tr("Shell"), this);
        connect(m_shapeToolShellAction, &QAction::triggered, this, &MainWindow::onShapeToolShell);
        m_shapeToolPannel->addLargeAction(m_shapeToolShellAction);

        m_shapeToolChamferAction = new QAction(QIcon(":/icons/icon/shape_tool_chamfer.png"), tr("Chamfer"), this);
        connect(m_shapeToolChamferAction, &QAction::triggered, this, &MainWindow::onShapeToolChamfer);
        m_shapeToolPannel->addLargeAction(m_shapeToolChamferAction);

        m_shapeToolFilletAction = new QAction(QIcon(":/icons/icon/shape_tool_fillet.svg"), tr("Fillet"), this);
        connect(m_shapeToolFilletAction, &QAction::triggered, this, &MainWindow::onShapeToolFillet);
        m_shapeToolPannel->addLargeAction(m_shapeToolFilletAction);

        m_shapeToolHoleAction = new QAction(QIcon(":/icons/icon/shape_tool_hole.svg"), tr("Hole"), this);
        connect(m_shapeToolHoleAction, &QAction::triggered, this, &MainWindow::onShapeToolHole);
        m_shapeToolPannel->addLargeAction(m_shapeToolHoleAction);
        return;
    };
    createShapeToolPannel();
}

void MainWindow::createCaeGroup()
{
    m_caeCategory = m_ribbon->addCategoryPage(tr("CAE"));

    m_caeStudyPannel = m_caeCategory->addPannel(tr("Study"));
    m_caeNewStaticAction = new QAction(QIcon(":/icons/icon/cae_static_study.svg"), tr("Static"), this);
    connect(m_caeNewStaticAction, &QAction::triggered, this, &MainWindow::onCaeNewStaticStudy);
    m_caeStudyPannel->addLargeAction(m_caeNewStaticAction);

    m_caeNewThermalAction = new QAction(QIcon(":/icons/icon/cae_thermal_study.svg"), tr("Thermal"), this);
    connect(m_caeNewThermalAction, &QAction::triggered, this, &MainWindow::onCaeNewThermalStudy);
    m_caeStudyPannel->addLargeAction(m_caeNewThermalAction);

    m_caeGeometryPannel = m_caeCategory->addPannel(tr("Geometry"));
    m_caeUseCurrentGeometryAction = new QAction(QIcon(":/icons/icon/cae_use_current_geometry.svg"), tr("Use Current"), this);
    connect(m_caeUseCurrentGeometryAction, &QAction::triggered, this, &MainWindow::onCaeUseCurrentGeometry);
    m_caeGeometryPannel->addLargeAction(m_caeUseCurrentGeometryAction);

    m_caeNamedSelectionAction = new QAction(QIcon(":/icons/icon/cae_named_selection.svg"), tr("Named Selection"), this);
    connect(m_caeNamedSelectionAction, &QAction::triggered, this, &MainWindow::onCaeCreateNamedSelection);
    m_caeGeometryPannel->addSmallAction(m_caeNamedSelectionAction);

    m_caeMaterialPannel = m_caeCategory->addPannel(tr("Material"));
    m_caeAssignMaterialAction = new QAction(QIcon(":/icons/icon/cae_assign_material.svg"), tr("Assign"), this);
    connect(m_caeAssignMaterialAction, &QAction::triggered, this, &MainWindow::onCaeAssignMaterial);
    m_caeMaterialPannel->addLargeAction(m_caeAssignMaterialAction);

    m_caeBoundaryPannel = m_caeCategory->addPannel(tr("Boundary"));
    m_caeFixedSupportAction = new QAction(QIcon(":/icons/icon/cae_fixed_support.svg"), tr("Fixed"), this);
    connect(m_caeFixedSupportAction, &QAction::triggered, this, &MainWindow::onCaeAddFixedSupport);
    m_caeBoundaryPannel->addLargeAction(m_caeFixedSupportAction);

    m_caeForceAction = new QAction(QIcon(":/icons/icon/cae_force.svg"), tr("Force"), this);
    connect(m_caeForceAction, &QAction::triggered, this, &MainWindow::onCaeAddForce);
    m_caeBoundaryPannel->addLargeAction(m_caeForceAction);

    m_caePressureAction = new QAction(QIcon(":/icons/icon/cae_pressure.svg"), tr("Pressure"), this);
    connect(m_caePressureAction, &QAction::triggered, this, &MainWindow::onCaeAddPressure);
    m_caeBoundaryPannel->addLargeAction(m_caePressureAction);

    m_caeMeshPannel = m_caeCategory->addPannel(tr("Mesh"));
    m_caeGenerateMeshAction = new QAction(QIcon(":/icons/icon/cae_generate_mesh.svg"), tr("Generate"), this);
    connect(m_caeGenerateMeshAction, &QAction::triggered, this, &MainWindow::onCaeGenerateMesh);
    m_caeMeshPannel->addLargeAction(m_caeGenerateMeshAction);

    m_caeSolvePannel = m_caeCategory->addPannel(tr("Solve"));
    m_caeRunSolverAction = new QAction(QIcon(":/icons/icon/cae_run_solver.svg"), tr("Run"), this);
    connect(m_caeRunSolverAction, &QAction::triggered, this, &MainWindow::onCaeRunSolver);
    m_caeSolvePannel->addLargeAction(m_caeRunSolverAction);

    m_caeResultsPannel = m_caeCategory->addPannel(tr("Results"));
    m_caeShowDisplacementAction = new QAction(QIcon(":/icons/icon/cae_result_displacement.svg"), tr("Displacement"), this);
    connect(m_caeShowDisplacementAction, &QAction::triggered, this, &MainWindow::onCaeShowDisplacement);
    m_caeResultsPannel->addSmallAction(m_caeShowDisplacementAction);

    m_caeShowStressAction = new QAction(QIcon(":/icons/icon/cae_result_stress.svg"), tr("Stress"), this);
    connect(m_caeShowStressAction, &QAction::triggered, this, &MainWindow::onCaeShowStress);
    m_caeResultsPannel->addSmallAction(m_caeShowStressAction);

    m_caeShowTemperatureAction = new QAction(QIcon(":/icons/icon/cae_result_temperature.svg"), tr("Temperature"), this);
    connect(m_caeShowTemperatureAction, &QAction::triggered, this, &MainWindow::onCaeShowTemperature);
    m_caeResultsPannel->addSmallAction(m_caeShowTemperatureAction);

    m_caeDeformationScaleAction = new QAction(QIcon(":/icons/icon/cae_deformation_scale.svg"), tr("Deformation Scale"), this);
    connect(m_caeDeformationScaleAction, &QAction::triggered, this, &MainWindow::onCaeSetDeformationScale);
    m_caeResultsPannel->addSmallAction(m_caeDeformationScaleAction);

    m_caeProbeResultAction = new QAction(QIcon(":/icons/icon/cae_result_probe.svg"), tr("Probe"), this);
    connect(m_caeProbeResultAction, &QAction::triggered, this, &MainWindow::onCaeProbeResult);
    m_caeResultsPannel->addSmallAction(m_caeProbeResultAction);

    m_caePickNodeAction = new QAction(QIcon(":/icons/icon/cae_pick_node.svg"), tr("Pick Node"), this);
    m_caePickNodeAction->setCheckable(true);
    connect(m_caePickNodeAction, &QAction::toggled, this, &MainWindow::onCaePickNodeToggled);
    m_caeResultsPannel->addSmallAction(m_caePickNodeAction);

    m_caeSettingsPannel = m_caeCategory->addPannel(tr("Settings"));
    m_caeSettingsAction = new QAction(QIcon(":/icons/icon/cae_settings.svg"), tr("Settings"), this);
    connect(m_caeSettingsAction, &QAction::triggered, this, &MainWindow::onCaeSettings);
    m_caeSettingsPannel->addLargeAction(m_caeSettingsAction);
}

void MainWindow::createHelpGroup()
{
    // ---- help Group ----
    m_helpCategory = m_ribbon->addCategoryPage(tr("Help"));
    // ---- language Pannel ----
    m_languagePannel = m_helpCategory->addPannel(tr("Language"));
    m_languageAction = new QAction(QIcon(":/icons/icon/help_language.svg"), tr("Switch Language"), this);
    connect(m_languageAction, &QAction::triggered, this, &MainWindow::onSwitchLanguage);
    m_languagePannel->addLargeAction(m_languageAction);

    // version pannel
    m_versionPannel = m_helpCategory->addPannel( tr("Version") );  // Adding to Analysis category for simplicity
    m_versionAction = new QAction(QIcon(":/icons/icon/version.svg"), tr("Version"), this);  // Assuming an icon path
    connect(m_versionAction, &QAction::triggered, this, &MainWindow::onVersion);
    m_versionPannel->addLargeAction(m_versionAction);

#if 0
    // test
    m_functionTestAction = new QAction(QIcon(":/icons/icon/version.svg"), tr("test"), this);  
    connect(m_functionTestAction, &QAction::triggered, this, &MainWindow::onFunctionTest);
    m_versionPannel->addLargeAction(m_functionTestAction);
#endif
}

void MainWindow::onNewFile()
{
    m_viewerWidget->clearAll();
    m_caeController->clearProject();
    refreshCaeTree();
}
void MainWindow::onOpenFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open CAD File", "", "STEP (*.step *.stp);;IGES (*.iges *.igs)");
    if (!filename.isEmpty())
    {
        m_caeController->clearProject();
        m_viewerWidget->loadModel(filename);
        refreshCaeTree();
    }
}

void MainWindow::onSaveFile()
{
    // TODO
}

void MainWindow::onSaveAsFile()
{
    // TODO
}

void MainWindow::onExportFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export File"),
                                                    QString(),
                                                    tr("STEP Files (*.step *.stp);;IGES Files (*.iges *.igs)"));
    if (!fileName.isEmpty()) {
        m_viewerWidget->exportModel(fileName);
    }
}

void MainWindow::onExportDxf()
{
    if (m_viewerWidget) {
        m_viewerWidget->exportDxf();
    }
}

void MainWindow::onExportDwg()
{
    if (m_viewerWidget) {
        m_viewerWidget->exportDwg();
    }
}

void MainWindow::onExport3dpdf()
{
    if (m_viewerWidget) {
        m_viewerWidget->export3dpdf();
    }
}

void MainWindow::onExportPicture()
{
    if (m_viewerWidget) {
        m_viewerWidget->exportPicture();
    }
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::onViewFit() const
{
    m_viewerWidget->viewFit();
}

void MainWindow::onChangeViewIsometric() const
{
    m_viewerWidget->viewIsometric();
}

void MainWindow::onChangeViewTop() const
{
    m_viewerWidget->viewTop();
}

void MainWindow::onChangeViewBottom() const
{
    m_viewerWidget->viewBottom();
}

void MainWindow::onChangeViewLeft() const
{
    m_viewerWidget->viewLeft();
}

void MainWindow::onChangeViewRight() const
{
    m_viewerWidget->viewRight();
}

void MainWindow::onChangeViewFront() const
{
    m_viewerWidget->viewFront();
}

void MainWindow::onChangeViewBack() const
{
    m_viewerWidget->viewBack();
}

void MainWindow::onSetDisplayMode(int mode) const
{
    m_viewerWidget->setDisplayMode(mode);
}

void MainWindow::onSwitchSelect(bool checked)
{
    m_viewerWidget->switchSelect(checked);
}

void MainWindow::onFilterStateChanged(const int filterType, const bool isChecked)
{
    m_viewerWidget->updateSelectionFilter(static_cast<TopAbs_ShapeEnum>(filterType), isChecked);
}

void MainWindow::onCheckInterference() const
{
    m_viewerWidget->checkInterference();
}

void MainWindow::onTransform() const
{
    //m_viewerWidget->transform();
    if (m_widgetTransform) {
        m_widgetTransform->show();
    }
}

void MainWindow::onClipping() const
{
    m_widgetClipping->show();
}

void MainWindow::onExplosion()
{
    if(!m_widgetExplodeAsm)
        return;
    m_widgetExplodeAsm->show();
    m_widgetExplodeAsm->adjustSize();
}

void MainWindow::onCreateWorkPlane()
{
    m_viewerWidget->createWorkPlane();
    if (m_widgetSetCoordinateSystem)
    {
        m_widgetSetCoordinateSystem->show();
    }
}

void MainWindow::onAnimation()
{
    m_viewerWidget->animation();
}

void MainWindow::onBusbar()
{
    m_viewerWidget->busbar();
}

void MainWindow::onMeasureDistance() const
{
    m_viewerWidget->measureDistance();
}

void MainWindow::onMeasureLength() const
{
    m_viewerWidget->measureLength();
}

void MainWindow::onMeasureArcLength() const
{
    m_viewerWidget->measureArcLength();
}

void MainWindow::onMeasureAngle() const
{
    m_viewerWidget->measureAngle();
}

void MainWindow::onMeasureMininumDistance() const
{
    m_viewerWidget->measureMinimumDistance();
}

void MainWindow::onCreatePoint()
{
    m_viewerWidget->createPoint();
}

void MainWindow::onCreateLine()
{
    m_viewerWidget->createLine();
}

void MainWindow::onCreateRectangle()
{
    m_viewerWidget->createRectangle();
}

void MainWindow::onCreateCircle()
{
    m_viewerWidget->createCircle();
}

void MainWindow::onCreateArc()
{
    m_viewerWidget->createArc();
}

void MainWindow::onCreateEllipse()
{
    m_viewerWidget->createEllipse();
}

void MainWindow::onCreatePolygon()
{
    m_viewerWidget->createPolygon();
}

void MainWindow::onCreateBezierCurve()
{
    m_viewerWidget->createBezierCurve();
}

void MainWindow::onCreateNurbsCurve()
{
    m_viewerWidget->createNurbsCurve();
}

void MainWindow::onCreateBox()
{
    m_viewerWidget->createBox();
}

void MainWindow::onCreatePyramid()
{
    m_viewerWidget->createPyramid();
}

void MainWindow::onCreateSphere()
{
    m_viewerWidget->createSphere();
}

void MainWindow::onCreateCylinder()
{
    m_viewerWidget->createCylinder();
}

void MainWindow::onCreateCone()
{
    m_viewerWidget->createCone();
}

void MainWindow::onBooleanOperationAction()
{
    m_viewerWidget->booleanOperation();
}

void MainWindow::onMirrorByPlane()
{
    m_viewerWidget->mirrorByPlane();
}

void MainWindow::onMirrorByAxis()
{
    m_viewerWidget->mirrorByAxis();
}

void MainWindow::setStatusText(StatusType type, const QString& text)
{
    if (m_statusLabels.contains(type)) {
        m_statusLabels[type]->setText(text);
    }
}

void MainWindow::updateCoordInfo(double x, double y, double z)
{
    static const int precision = 2;
    QString text = QString("X: %1 Y: %2 Z: %3")
                   .arg(x, 0, 'f', precision)
                   .arg(y, 0, 'f', precision)
                   .arg(z, 0, 'f', precision);
    setStatusText(StatusCoord, text);
}

void MainWindow::updateShapeInfo(const QString& info)
{
    setStatusText(StatusShapeInfo, info);
}

void MainWindow::onPatternLinear()
{
    m_viewerWidget->patternLinear();
}

void MainWindow::onPatternCircular()
{
    m_viewerWidget->patternCircular();
}

void MainWindow::onShapeToolShell()
{
    m_viewerWidget->shell();
}

void MainWindow::onShapeToolChamfer()
{
    m_viewerWidget->chamfer();
}

void MainWindow::onShapeToolFillet()
{
    m_viewerWidget->fillet();
}

void MainWindow::onShapeToolHole()
{
    m_viewerWidget->hole();
}

void MainWindow::onCaeNewStaticStudy()
{
    m_caePickNodeAction->setChecked(false);
    m_currentCaeResultField.reset();
    m_viewerWidget->clearScalarField();
    m_viewerWidget->clearCaeMesh();
    m_viewerWidget->clearCaeBoundaryMarkers();
    updateStatusMessage(m_caeController->execute(std::make_unique<Cae::CreateStudyCommand>(Cae::StudyType::StaticStructural)), 5000);
    refreshCaeTree();
}

void MainWindow::onCaeNewThermalStudy()
{
    m_caePickNodeAction->setChecked(false);
    m_currentCaeResultField.reset();
    m_viewerWidget->clearScalarField();
    m_viewerWidget->clearCaeMesh();
    m_viewerWidget->clearCaeBoundaryMarkers();
    updateStatusMessage(m_caeController->execute(std::make_unique<Cae::CreateStudyCommand>(Cae::StudyType::SteadyThermal)), 5000);
    refreshCaeTree();
}

void MainWindow::onCaeUseCurrentGeometry()
{
    m_caePickNodeAction->setChecked(false);
    m_currentCaeResultField.reset();
    m_viewerWidget->clearScalarField();
    m_viewerWidget->clearCaeMesh();
    m_viewerWidget->clearCaeBoundaryMarkers();
    updateStatusMessage(m_caeController->useCurrentGeometry(m_viewerWidget->hasGeometry()), 5000);
    refreshCaeTree();
}

void MainWindow::onCaeCreateNamedSelection()
{
    Cae::PlanarSelectionRegion region;
    QString errorMessage;
    if (!m_viewerWidget->selectedCaePlanarFace(&region, &errorMessage)) {
        m_viewerWidget->setFilters({
            {TopAbs_VERTEX, false},
            {TopAbs_EDGE, false},
            {TopAbs_FACE, true},
            {TopAbs_SOLID, false}});
        updateStatusMessage(
            tr("%1 Face selection mode is now active; select one face and click Named Selection again.")
                .arg(errorMessage),
            8000);
        return;
    }

    const Cae::CaeStudy* study = m_caeController->project().activeStudy();
    const int faceSelectionCount = study
        ? static_cast<int>(std::count_if(
              study->namedSelections().cbegin(),
              study->namedSelections().cend(),
              [](const Cae::CaeNamedSelection& selection) {
                  return selection.scope() == Cae::NamedSelectionScope::Face;
              }))
        : 0;
    bool accepted = false;
    const QString name = QInputDialog::getText(
        this,
        tr("Create Named Selection"),
        tr("Selection name:"),
        QLineEdit::Normal,
        tr("Face Selection %1").arg(faceSelectionCount + 1),
        &accepted);
    if (!accepted || name.trimmed().isEmpty()) {
        return;
    }

    updateStatusMessage(m_caeController->createNamedSelection(name, region), 5000);
    resetCaeResultPresentation(false);
    refreshCaeTree();
}

void MainWindow::onCaeAssignMaterial()
{
    DialogCaeMaterial dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    updateStatusMessage(
        m_caeController->assignMaterial(
            dialog.materialName(),
            dialog.youngModulus(),
            dialog.poissonRatio()),
        5000);
    resetCaeResultPresentation(false);
    refreshCaeTree();
}

void MainWindow::onCaeAddFixedSupport()
{
    bool accepted = false;
    const QString targetName = chooseCaeFaceTarget(tr("Fixed Support Target"), &accepted);
    if (!accepted) {
        return;
    }
    updateStatusMessage(m_caeController->addFixedSupport(targetName), 5000);
    resetCaeResultPresentation(true);
    refreshCaeBoundaryVisualization();
    refreshCaeTree();
}

void MainWindow::onCaeAddForce()
{
    bool targetAccepted = false;
    const QString targetName = chooseCaeFaceTarget(tr("Force Target"), &targetAccepted);
    if (!targetAccepted) {
        return;
    }

    DialogCaeForce dialog(m_caeForceComponents, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_caeForceComponents = dialog.force();
    updateStatusMessage(
        m_caeController->addForce(m_caeForceComponents, targetName),
        5000);
    resetCaeResultPresentation(true);
    refreshCaeBoundaryVisualization();
    refreshCaeTree();
}

void MainWindow::onCaeAddPressure()
{
    bool targetAccepted = false;
    const QString targetName = chooseCaeFaceTarget(tr("Pressure Target"), &targetAccepted);
    if (!targetAccepted) {
        return;
    }

    bool accepted = false;
    const double pressure = QInputDialog::getDouble(
        this,
        tr("Add CAE Pressure"),
        tr("Pressure (MPa):"),
        m_caePressureValue,
        1.0e-12,
        1.0e12,
        6,
        &accepted);
    if (!accepted) {
        return;
    }
    m_caePressureValue = pressure;
    updateStatusMessage(
        m_caeController->addPressure(pressure, targetName),
        5000);
    resetCaeResultPresentation(true);
    refreshCaeBoundaryVisualization();
    refreshCaeTree();
}

QString MainWindow::chooseCaeFaceTarget(const QString& title, bool* accepted)
{
    if (accepted) {
        *accepted = false;
    }

    const Cae::CaeStudy* study = m_caeController->project().activeStudy();
    QStringList faceNames;
    if (study) {
        for (const Cae::CaeNamedSelection& selection : study->namedSelections()) {
            if (selection.scope() == Cae::NamedSelectionScope::Face &&
                selection.planarRegion()) {
                faceNames.push_back(selection.name());
            }
        }
    }
    if (faceNames.isEmpty()) {
        updateStatusMessage(
            tr("Create at least one planar face Named Selection first."),
            8000);
        return QString();
    }
    if (faceNames.size() == 1) {
        if (accepted) {
            *accepted = true;
        }
        return faceNames.front();
    }

    bool itemAccepted = false;
    const QString targetName = QInputDialog::getItem(
        this,
        title,
        tr("Named selection:"),
        faceNames,
        0,
        false,
        &itemAccepted);
    if (accepted) {
        *accepted = itemAccepted;
    }
    return targetName;
}

void MainWindow::resetCaeResultPresentation(bool preserveMesh)
{
    if (m_caePickNodeAction) {
        m_caePickNodeAction->setChecked(false);
    }
    m_currentCaeResultField.reset();
    m_viewerWidget->clearScalarField();

    const Cae::CaeStudy* study = m_caeController->project().activeStudy();
    if (preserveMesh && study && study->mesh() && QFileInfo::exists(study->mesh()->source())) {
        QString errorMessage;
        if (!m_viewerWidget->showCaeMesh(study->mesh()->source(), &errorMessage)) {
            updateStatusMessage(errorMessage, 5000);
        }
        return;
    }
    m_viewerWidget->clearCaeMesh();
}

void MainWindow::onCaeGenerateMesh()
{
    bool accepted = false;
    const double globalSize = QInputDialog::getDouble(
        this,
        tr("Generate CAE Mesh"),
        tr("Maximum element size:"),
        m_caeGlobalMeshSize,
        1.0e-6,
        1.0e9,
        6,
        &accepted);
    if (!accepted) {
        return;
    }
    m_caeGlobalMeshSize = globalSize;
    m_caePickNodeAction->setChecked(false);
    m_currentCaeResultField.reset();
    QString geometryFilePath;
    const Cae::CaeExternalToolConfig& config = m_caeController->externalToolConfig();
    if (config.hasExecutablePath(Cae::ExternalTool::Gmsh)) {
        QString workingDirectory = config.workingDirectory();
        if (workingDirectory.isEmpty()) {
            workingDirectory = QDir::temp().filePath(QStringLiteral("Qt_OCC_CAE"));
        }
        if (!QDir().mkpath(workingDirectory)) {
            updateStatusMessage(tr("Failed to create CAE working directory: %1").arg(workingDirectory), 8000);
            return;
        }

        geometryFilePath = QDir(workingDirectory).filePath(QStringLiteral("cae_geometry.step"));
        QString exportError;
        if (!m_viewerWidget->exportCaeGeometry(geometryFilePath, &exportError)) {
            updateStatusMessage(exportError, 8000);
            return;
        }
    }

    const QString meshMessage = m_caeController->generateMesh(geometryFilePath, globalSize);
    updateStatusMessage(meshMessage, 8000);
    refreshCaeTree();

    const Cae::CaeStudy* study = m_caeController->project().activeStudy();
    if (!study || !study->mesh()) {
        return;
    }
    const QString meshFilePath = study->mesh()->source();
    if (!QFileInfo::exists(meshFilePath)) {
        return;
    }

    QString displayError;
    if (!m_viewerWidget->showCaeMesh(meshFilePath, &displayError)) {
        updateStatusMessage(displayError, 8000);
    } else {
        refreshCaeBoundaryVisualization();
    }
}

void MainWindow::onCaeRunSolver()
{
    m_caePickNodeAction->setChecked(false);
    m_currentCaeResultField.reset();
    updateStatusMessage(m_caeController->runSolver(), 5000);
    refreshCaeTree();
}

void MainWindow::onCaeShowDisplacement()
{
    presentCaeResult(Cae::ResultFieldType::Displacement);
}

void MainWindow::onCaeShowStress()
{
    presentCaeResult(Cae::ResultFieldType::VonMisesStress);
}

void MainWindow::onCaeShowTemperature()
{
    presentCaeResult(Cae::ResultFieldType::Temperature);
}

void MainWindow::onCaeTreeMeshActivated(const QUuid& studyId)
{
    if (!m_caeController->activateStudy(studyId)) {
        updateStatusMessage(tr("The selected CAE study is unavailable."), 5000);
        return;
    }
    refreshCaeTree();

    const Cae::CaeStudy* study = m_caeController->project().activeStudy();
    if (!study || !study->mesh()) {
        updateStatusMessage(tr("The active CAE study has no mesh to display."), 5000);
        return;
    }

    resetCaeResultPresentation(true);
    refreshCaeBoundaryVisualization();
    updateStatusMessage(tr("CAE mesh displayed."), 3000);
}

void MainWindow::onCaeTreeResultActivated(
    const QUuid& studyId,
    Cae::ResultFieldType fieldType)
{
    if (!m_caeController->activateStudy(studyId)) {
        updateStatusMessage(tr("The selected CAE study is unavailable."), 5000);
        return;
    }
    refreshCaeTree();
    presentCaeResult(fieldType, false);
}

void MainWindow::onCaeRemoveBoundaryConditionRequested(
    const QUuid& studyId,
    const QString& name)
{
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        tr("Delete CAE Boundary Condition"),
        tr("Delete \"%1\"?\nExisting solution results for this study will be cleared.")
            .arg(name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    updateStatusMessage(
        m_caeController->removeBoundaryCondition(studyId, name),
        5000);
    resetCaeResultPresentation(true);
    refreshCaeBoundaryVisualization();
    refreshCaeTree();
}

void MainWindow::onCaeSetDeformationScale()
{
    bool accepted = false;
    const double scale = QInputDialog::getDouble(
        this,
        tr("CAE Deformation Scale"),
        tr("Scale factor (0 = Auto):"),
        m_caeDeformationScale,
        0.0,
        1.0e6,
        2,
        &accepted);
    if (!accepted) {
        return;
    }

    m_caeDeformationScale = scale;
    if (m_currentCaeResultField) {
        presentCaeResult(*m_currentCaeResultField, false);
    } else {
        updateStatusMessage(tr("Select a CAE result field before setting deformation scale."), 5000);
    }
}

void MainWindow::onCaeProbeResult()
{
    if (!m_currentCaeResultField) {
        updateStatusMessage(tr("Display a CAE result field before probing a node."), 5000);
        return;
    }

    const Cae::CaeStudy* study = m_caeController->project().activeStudy();
    if (!study || !study->result()) {
        updateStatusMessage(tr("No CAE result is available for probing."), 5000);
        return;
    }

    const Cae::CaeResultField* field = study->result()->field(*m_currentCaeResultField);
    if (!field || field->nodalValues().empty()) {
        updateStatusMessage(tr("The current result field has no nodal values."), 5000);
        return;
    }

    const int minimumNodeId = field->nodalValues().cbegin()->first;
    const int maximumNodeId = field->nodalValues().crbegin()->first;
    const int initialNodeId = m_caeProbeNodeId >= minimumNodeId && m_caeProbeNodeId <= maximumNodeId
        ? m_caeProbeNodeId
        : minimumNodeId;
    bool accepted = false;
    const int nodeId = QInputDialog::getInt(
        this,
        tr("CAE Result Probe"),
        tr("Node ID (%1 - %2):").arg(minimumNodeId).arg(maximumNodeId),
        initialNodeId,
        minimumNodeId,
        maximumNodeId,
        1,
        &accepted);
    if (!accepted) {
        return;
    }

    showCaeNodeProbe(nodeId);
}

void MainWindow::onCaePickNodeToggled(bool enabled)
{
    if (enabled && !m_currentCaeResultField) {
        const QSignalBlocker blocker(m_caePickNodeAction);
        m_caePickNodeAction->setChecked(false);
        updateStatusMessage(tr("Display a nodal CAE result before enabling node picking."), 5000);
        return;
    }

    QString errorMessage;
    if (!m_viewerWidget->setCaeNodePickingEnabled(enabled, &errorMessage)) {
        const QSignalBlocker blocker(m_caePickNodeAction);
        m_caePickNodeAction->setChecked(false);
        updateStatusMessage(errorMessage, 5000);
        return;
    }

    updateStatusMessage(
        enabled ? tr("Node picking enabled. Click a displayed mesh node.")
                : tr("Node picking disabled."),
        5000);
}

void MainWindow::onCaeNodePicked(int nodeId)
{
    if (m_caePickNodeAction && m_caePickNodeAction->isChecked()) {
        showCaeNodeProbe(nodeId);
    }
}

void MainWindow::showCaeNodeProbe(int nodeId)
{
    if (!m_currentCaeResultField) {
        updateStatusMessage(tr("Display a CAE result field before probing a node."), 5000);
        return;
    }

    const Cae::CaeStudy* study = m_caeController->project().activeStudy();
    if (!study || !study->result()) {
        updateStatusMessage(tr("No CAE result is available for probing."), 5000);
        return;
    }

    const Cae::CaeResultField* field = study->result()->field(*m_currentCaeResultField);
    if (!field) {
        updateStatusMessage(tr("The current CAE result field is unavailable."), 5000);
        return;
    }

    const auto probe = field->probeNode(nodeId);
    if (!probe) {
        updateStatusMessage(tr("Node %1 has no value in the current result field.").arg(nodeId), 5000);
        return;
    }

    m_caeProbeNodeId = nodeId;
    QString details = tr("Node: %1\nField: %2\nValue: %3 %4")
        .arg(probe->nodeId)
        .arg(Cae::toDisplayString(field->type()))
        .arg(QString::number(probe->value, 'g', 12))
        .arg(field->unit());
    if (probe->displacement) {
        const auto& displacement = *probe->displacement;
        details += tr("\nUx: %1 %4\nUy: %2 %4\nUz: %3 %4")
            .arg(QString::number(displacement[0], 'g', 12))
            .arg(QString::number(displacement[1], 'g', 12))
            .arg(QString::number(displacement[2], 'g', 12))
            .arg(field->unit());
    }
    QMessageBox::information(this, tr("CAE Result Probe"), details);
}

void MainWindow::presentCaeResult(Cae::ResultFieldType fieldType, bool reloadField)
{
    if (reloadField) {
        updateStatusMessage(m_caeController->showResult(fieldType), 5000);
        refreshCaeTree();
    }

    const Cae::CaeStudy* study = m_caeController->project().activeStudy();
    if (!study || !study->result()) {
        return;
    }

    const Cae::CaeResultField* field = study->result()->field(fieldType);
    if (!field) {
        return;
    }
    m_currentCaeResultField = fieldType;
    m_viewerWidget->clearCaeBoundaryMarkers();

    QString errorMessage;
    const QString title = QStringLiteral("%1 (%2)").arg(Cae::toDisplayString(fieldType), field->unit());
    bool displayed = false;
    if (!field->nodalValues().empty() && study->mesh() && QFileInfo::exists(study->mesh()->source())) {
        displayed = m_viewerWidget->showCaeScalarField(
            study->mesh()->source(),
            title,
            field->nodalValues(),
            field->nodalDisplacements(),
            m_caeDeformationScale,
            field->minValue(),
            field->maxValue(),
            &errorMessage);
    } else {
        displayed = m_viewerWidget->showScalarField(
            title,
            field->minValue(),
            field->maxValue(),
            &errorMessage);
    }
    if (!displayed) {
        updateStatusMessage(errorMessage, 5000);
    }
}

void MainWindow::onCaeSettings()
{
    DialogCaeSettings dialog(m_caeController->externalToolConfig(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_caeController->setExternalToolConfig(dialog.config());
    const Cae::CaeExternalToolConfig& config = m_caeController->externalToolConfig();
    Cae::CaeExternalToolConfigStore().save(config);
    QStringList configuredTools;
    QStringList missingTools;

    const auto collectToolStatus = [&](Cae::ExternalTool tool) {
        if (config.hasExecutablePath(tool)) {
            configuredTools << Cae::toDisplayString(tool);
        } else {
            missingTools << Cae::toDisplayString(tool);
        }
    };

    collectToolStatus(Cae::ExternalTool::Gmsh);
    collectToolStatus(Cae::ExternalTool::CalculiX);
    collectToolStatus(Cae::ExternalTool::GetDP);

    const QString workingDirectory = config.workingDirectory().isEmpty()
        ? tr("not set")
        : config.workingDirectory();
    const QString configuredText = configuredTools.isEmpty()
        ? tr("none")
        : configuredTools.join(QStringLiteral(", "));

    updateStatusMessage(
        tr("CAE tools configured: %1; missing: %2; work dir: %3; timeout: %4 ms.")
            .arg(configuredText,
                 missingTools.join(QStringLiteral(", ")),
                 workingDirectory)
            .arg(config.timeoutMilliseconds()),
        8000);
}

ViewerWidget* MainWindow::GetViewerWidget() const
{
    return m_viewerWidget;
}

ModelTreeWidget* MainWindow::GetModelTreeWidget() const
{
    return m_modelTreeWidget;
}

void MainWindow::updateStatusMessage(const QString& msg, int timeout)
{
    statusBar()->showMessage(msg, timeout);
}

void MainWindow::onSwitchLanguage()
{
    m_currentLanguage = (m_currentLanguage + 1) % 3;
    QString langCode;
    switch (m_currentLanguage)
    {
    case 0:
        langCode = "en";
        break;
    case 1:
        langCode = "zh";
        break;
    case 2:
        langCode = "ja";
        break;
    }

    QCoreApplication::removeTranslator(m_translator);
    QString qmFile = QString(":/translations/qt_occ_%1.qm").arg(langCode);
    if (m_translator->load(qmFile)) {
        QCoreApplication::installTranslator(m_translator);
        // Re-create the UI to apply the new language
        setupUi();
    }
}

void MainWindow::createThemeActions()
{
    m_themePannel = m_helpCategory->addPannel(tr("Theme"));

    m_themeMenu = new QMenu(this);
    m_lightThemeAction = new QAction(tr("Light"), this);
    m_darkThemeAction = new QAction(tr("Dark"), this);

    m_themeMenu->addAction(m_lightThemeAction);
    m_themeMenu->addAction(m_darkThemeAction);

    connect(m_lightThemeAction, &QAction::triggered, this, &MainWindow::onSwitchTheme);
    connect(m_darkThemeAction, &QAction::triggered, this, &MainWindow::onSwitchTheme);

    QAction* themeAction = new QAction(QIcon(":/icons/icon/help_theme.svg"), tr("Switch Theme"),this);
    themeAction->setMenu(m_themeMenu);
    m_themePannel->addLargeAction(themeAction);
}

void MainWindow::onSwitchTheme()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action)
    {
        return;
    }

    QString themePath;
    if (action == m_lightThemeAction)
    {
        themePath = ":/qss/qss/light.qss";
    }
    else if (action == m_darkThemeAction)
    {
        themePath = ":/qss/qss/dark.qss";
    }

    QFile file(themePath);
    if (file.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream stream(&file);
        QString styleSheet = stream.readAll();
        qApp->setStyleSheet(styleSheet);
        file.close();
    }
}


void MainWindow::onVersion()
{
    DialogAbout dlg(this);
    dlg.exec();
}

void MainWindow::onFunctionTest()
{
    m_viewerWidget->onFunctionTest();
}
