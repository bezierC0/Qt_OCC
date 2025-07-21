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
#include <TopoDS_Shape.hxx> // This should be resolved by CMakeLists.txt fix
#include <TopAbs_ShapeEnum.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include "ViewerWidget.h"
#include "WidgetModelTree.h"
#include "DialogAbout.h"
#include "widget_explode_assembly.h"


MainWindow::MainWindow(QWidget* parent) : SARibbonMainWindow(parent)
{
    m_translator = new QTranslator(this);
    m_currentLanguage = 0; // 0: English, 1: Chinese, 2: Japanese

    m_modelTreeWidget = new ModelTreeWidget( this );
    m_viewerWidget = new ViewerWidget(this);

    const auto splitter = new QSplitter( Qt::Horizontal, this );
    splitter->addWidget( m_modelTreeWidget );
    splitter->addWidget( m_viewerWidget );

    splitter->setStretchFactor( 0, 1 ); // m_modelTreeWidget
    splitter->setStretchFactor( 1, 4 ); 

    setCentralWidget( splitter );

    setupUi();

    resize(1200, 800);
}

void MainWindow::setupUi()
{
    createRibbon();
    createFileGroup();
    createViewGroup();
    createToolGroup();
    createShapeGroup();
    createHelpGroup();

    m_widgetExplodeAsm = new WidgetExplodeAssembly();
    createThemeActions();
}

void MainWindow::createRibbon() {
    if ( m_ribbon )
    {
        m_ribbon->clear();
        m_ribbon->removeCategory( m_fileCategory );
        m_ribbon->removeCategory( m_viewCategory );
        m_ribbon->removeCategory( m_toolCategory );
        m_ribbon->removeCategory( m_shapeCategory );
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
    //connect(m_newAction, &QAction::triggered, this, &MainWindow::newFile);
    m_filePannel->addLargeAction(m_newAction);

    // open
    m_openAction = new QAction(QIcon(":/icons/icon/open.png"), tr("Open"), this); // Assuming an icon path
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    m_filePannel->addLargeAction(m_openAction);
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

        auto createDisplayAction = [&](QIcon& icon,const QString& text, int mode) {
            //auto action = new QAction(icon,text, this);
            auto action = new QAction(text, this);
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
        auto shadingEdgeAction = createDisplayAction(QIcon(":/icons/icon/display_mode_shading_edge.svg"),tr("Shading with edge"), 10);

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
        m_measureDistanceAction = new QAction(QIcon(":/icons/icon/measure_distance.svg"), tr("MeasureDistance"), this);
        connect(m_measureDistanceAction, &QAction::triggered, this, &MainWindow::onMeasureDistance);
        m_measurePannel->addSmallAction(m_measureDistanceAction);

        // measure length
        m_measureLengthAction = new QAction(QIcon(":/icons/icon/measure_length.svg"), tr("MeasureLength"), this);
        connect(m_measureLengthAction, &QAction::triggered, this, &MainWindow::onMeasureLength);
        m_measurePannel->addSmallAction(m_measureLengthAction);

        // measure arc length
        m_measureArcLengthAction = new QAction(QIcon(":/icons/icon/measure_arc_length.svg"), tr("MeasureArcLength"), this);
        connect(m_measureArcLengthAction, &QAction::triggered, this, &MainWindow::onMeasureArcLength);
        m_measurePannel->addSmallAction(m_measureArcLengthAction);

        // measure angle
        m_measureAngleAction = new QAction(QIcon(":/icons/icon/measure_angle.svg"), tr("MeasureAngle"), this); 
        connect(m_measureAngleAction, &QAction::triggered, this, &MainWindow::onMeasureAngle);
        m_measurePannel->addSmallAction(m_measureAngleAction);
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

        // boolean union
        m_booleanUnionAction = new QAction(QIcon(":/icons/icon/boolean_union.svg"), tr("Union"), this);
        connect(m_booleanUnionAction, &QAction::triggered, this, &MainWindow::onBooleanUnionAction);
        m_shapeBooleanPannel->addLargeAction(m_booleanUnionAction);

        // boolean Intersection
        m_booleanIntersectionAction = new QAction(QIcon(":/icons/icon/boolean_intersection.svg"), tr("Intersection"), this);
        connect(m_booleanIntersectionAction, &QAction::triggered, this, &MainWindow::onBooleanIntersection);
        m_shapeBooleanPannel->addLargeAction(m_booleanIntersectionAction);

        // boolean difference
        m_booleanDifferenceAction = new QAction(QIcon(":/icons/icon/boolean_difference.svg"), tr("Difference"), this);
        connect(m_booleanDifferenceAction, &QAction::triggered, this, &MainWindow::onBooleanDifference);
        m_shapeBooleanPannel->addLargeAction(m_booleanDifferenceAction);
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
}

void MainWindow::onNewFile()
{
    // TODO
}
void MainWindow::onOpenFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open CAD File", "", "STEP (*.step *.stp);;IGES (*.iges *.igs)");
    if (!filename.isEmpty())
    {
        m_viewerWidget->loadModel(filename);
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
    // TODO
}

void MainWindow::onExit()
{
    // TODO
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
    m_viewerWidget->transform();
}

void MainWindow::onClipping() const
{
    const gp_Dir normal(0.0, 0.0, 1.0);
    const gp_Pnt point(0.0, 0.0, 10.0);
    m_viewerWidget->clipping( normal , point );
}

void MainWindow::onExplosion()
{
    if(!m_widgetExplodeAsm)
        return;
    m_widgetExplodeAsm->show();
    m_widgetExplodeAsm->adjustSize();
}

void MainWindow::onMeasureDistance() const
{
    m_viewerWidget->measureDistance();
}

void MainWindow::onMeasureLength() const
{
    // TODO
}

void MainWindow::onMeasureArcLength() const
{
    // TODO
}

void MainWindow::onMeasureAngle() const
{
    // TODO
}

void MainWindow::onVersion()
{
    DialogAbout dlg(this);
    dlg.exec();
}

void MainWindow::onCreatePoint()
{
    gp_Pnt p(10, 20, 30);
    BRepBuilderAPI_MakeVertex vertexMaker(p);
    if (vertexMaker.IsDone())
    {
        m_viewerWidget->displayShape(vertexMaker.Shape(), 1.0, 1.0, 1.0); 
    }
}

void MainWindow::onCreateLine()
{
    gp_Pnt p1(0, 0, 0);
    gp_Pnt p2(50, 50, 50);
    BRepBuilderAPI_MakeEdge edge(p1, p2);
    if (edge.IsDone())
    {
        m_viewerWidget->displayShape(edge.Shape(), 1.0, 0.0, 0.0);
    }
}

void MainWindow::onCreateRectangle()
{
    gp_Pnt p1(0, 0, 0);
    gp_Pnt p2(40, 0, 0);
    gp_Pnt p3(40, 30, 0);
    gp_Pnt p4(0, 30, 0);
    BRepBuilderAPI_MakePolygon poly;
    poly.Add(p1);
    poly.Add(p2);
    poly.Add(p3);
    poly.Add(p4);
    poly.Add(p1); // Close the polygon
    if (poly.IsDone())
    {
        BRepBuilderAPI_MakeFace face(poly.Wire());
        if (face.IsDone())
        {
            m_viewerWidget->displayShape(face.Shape(), 0.0, 1.0, 0.0);
        }
    }
}

void MainWindow::onCreateCircle()
{
    gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); // Z axis
    gp_Circ circle(axis, 25.0);                   // Radius 25
    BRepBuilderAPI_MakeEdge edge(circle);
    if (edge.IsDone())
    {
        m_viewerWidget->displayShape(edge.Shape(), 0.0, 0.0, 1.0);
    }
}

void MainWindow::onCreateArc()
{
    gp_Pnt center(0, 0, 0);
    gp_Pnt p1(30, 0, 0);
    gp_Pnt p2(0, 30, 0);
    GC_MakeArcOfCircle arc(center, p1, p2);
    if (arc.IsDone())
    {
        BRepBuilderAPI_MakeEdge edge(arc.Value());
        if (edge.IsDone())
        {
            m_viewerWidget->displayShape(edge.Shape(), 1.0, 1.0, 0.0);
        }
    }
}

void MainWindow::onCreateEllipse()
{
    gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); // Z axis
    gp_Elips ellipse(axis, 30.0, 15.0);           // Major radius 30, minor 15
    BRepBuilderAPI_MakeEdge edge(ellipse);
    if (edge.IsDone())
    {
        m_viewerWidget->displayShape(edge.Shape(), 1.0, 0.0, 1.0);
    }
}

void MainWindow::onCreatePolygon()
{
    BRepBuilderAPI_MakePolygon poly;
    poly.Add(gp_Pnt(0, 0, 0));
    poly.Add(gp_Pnt(20, 10, 0));
    poly.Add(gp_Pnt(30, 30, 0));
    poly.Add(gp_Pnt(10, 40, 0));
    poly.Add(gp_Pnt(-10, 20, 0));
    poly.Add(gp_Pnt(0, 0, 0)); // Close it
    if (poly.IsDone())
    {
        m_viewerWidget->displayShape(poly.Wire(), 0.0, 1.0, 1.0);
    }
}

void MainWindow::onCreateBezierCurve()
{
    TColgp_Array1OfPnt poles(1, 4);
    poles.SetValue(1, gp_Pnt(0, 0, 0));
    poles.SetValue(2, gp_Pnt(10, 40, 0));
    poles.SetValue(3, gp_Pnt(40, 40, 0));
    poles.SetValue(4, gp_Pnt(50, 0, 0));
    Handle(Geom_BezierCurve) bezier = new Geom_BezierCurve(poles);
    BRepBuilderAPI_MakeEdge edge(bezier);
    if (edge.IsDone())
    {
        m_viewerWidget->displayShape(edge.Shape(), 0.5, 0.5, 0.5);
    }
}

void MainWindow::onCreateNurbsCurve()
{
    TColgp_Array1OfPnt poles(1, 4);
    poles.SetValue(1, gp_Pnt(0, 0, 0));
    poles.SetValue(2, gp_Pnt(10, 40, 0));
    poles.SetValue(3, gp_Pnt(40, -40, 0));
    poles.SetValue(4, gp_Pnt(50, 0, 0));

    TColStd_Array1OfReal knots(1, 2);
    knots.SetValue(1, 0.0);
    knots.SetValue(2, 1.0);

    TColStd_Array1OfInteger mults(1, 2);
    mults.SetValue(1, 4);
    mults.SetValue(2, 4);

    Standard_Integer degree = 3;

    Handle(Geom_BSplineCurve) bspline = new Geom_BSplineCurve(poles, knots, mults, degree);
    BRepBuilderAPI_MakeEdge edge(bspline);
    if (edge.IsDone())
    {
        m_viewerWidget->displayShape(edge.Shape(), 0.2, 0.8, 0.2);
    }
}

void MainWindow::onCreateBox()
{
    BRepPrimAPI_MakeBox box(10.0, 20.0, 30.0);
    m_viewerWidget->displayShape(box.Shape(), 1.0, 0.0, 1.0);
}

void MainWindow::onCreatePyramid()
{
    m_viewerWidget->createPyramid();
}

void MainWindow::onCreateSphere()
{
    BRepPrimAPI_MakeSphere sphere( gp_Pnt( 0, 0, 0 ), 5.0 ) ;
    m_viewerWidget->displayShape( sphere.Shape(), 1.0, 0.0, 0.0 ) ;  // NOLINT
}

void MainWindow::onCreateCylinder()
{
    BRepPrimAPI_MakeCylinder cylinder(5.0, 10.0);
    m_viewerWidget->displayShape(cylinder.Shape(), 0.0, 1.0, 0.0);
}

void MainWindow::onCreateCone()
{
    BRepPrimAPI_MakeCone cone(5.0, 0.0, 10.0);
    m_viewerWidget->displayShape(cone.Shape(), 0.0, 1.0, 1.0);
}

void MainWindow::onBooleanUnionAction()
{
    m_viewerWidget->booleanUnion();
}

void MainWindow::onBooleanIntersection()
{
    m_viewerWidget->booleanIntersection();
}

void MainWindow::onBooleanDifference()
{
    m_viewerWidget->booleanDifference();
}

void MainWindow::onMirrorByPlane()
{
    m_viewerWidget->mirrorByPlane();
}

void MainWindow::onMirrorByAxis()
{
    m_viewerWidget->mirrorByAxis();
}

void MainWindow::onPatternLinear()
{
    m_viewerWidget->patternLinear();
}

void MainWindow::onPatternCircular()
{
    m_viewerWidget->patternCircular();
}

ViewerWidget* MainWindow::GetViewerWidget() const
{
    return m_viewerWidget;
}

ModelTreeWidget* MainWindow::GetModelTreeWidget() const
{
    return m_modelTreeWidget;
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
