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
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFile);
    m_filePannel->addLargeAction(m_openAction);
}

void MainWindow::createViewGroup()
{
    // ---- View Group ----
    m_viewCategory = m_ribbon->addCategoryPage(tr("View"));
    m_viewPannel = m_viewCategory->addPannel(tr("View Operations"));
    m_fitAction = new QAction(QIcon(":/icons/icon/fit.png"), tr("Fit"), this); // Assuming an icon path
    connect(m_fitAction, &QAction::triggered, this, &MainWindow::viewFit);
    m_viewPannel->addLargeAction(m_fitAction);

    // select
    m_selectAction = new QAction(QIcon(":/icons/icon/select.svg"), tr("Select"), this);
    m_selectAction->setCheckable(true);
    connect(m_selectAction, &QAction::toggled, this, &MainWindow::onSelectModeToggled);
    m_viewPannel->addLargeAction(m_selectAction);

    // Select Filter
    QToolButton* selectFilterButton = new QToolButton(this);
    selectFilterButton->setText(tr("Select Filter"));
    selectFilterButton->setIcon(QIcon(":/icons/icon/select_filter.svg"));
    selectFilterButton->setPopupMode(QToolButton::MenuButtonPopup);

    QMenu* filterMenu = new QMenu(selectFilterButton);

    // Create checkboxes for each filter type
    QCheckBox* vertexCheckBox = new QCheckBox(tr("Vertices"), filterMenu);
    QCheckBox* edgeCheckBox = new QCheckBox(tr("Edges"), filterMenu);
    QCheckBox* faceCheckBox = new QCheckBox(tr("Faces"), filterMenu);
    QCheckBox* solidCheckBox = new QCheckBox(tr("Solids"), filterMenu);

    // Set initial checkbox states based on current filters
    const auto& currentFilters = m_viewerWidget->getSelectionFilters();
    vertexCheckBox->setChecked(currentFilters.at(TopAbs_VERTEX));
    edgeCheckBox->setChecked(currentFilters.at(TopAbs_EDGE));
    faceCheckBox->setChecked(currentFilters.at(TopAbs_FACE));
    solidCheckBox->setChecked(currentFilters.at(TopAbs_SOLID));

    // Create actions for checkboxes and make them checkable
    auto addCheckableAction = [&](QCheckBox* checkBox) {
        QAction* action = new QAction(checkBox->text(), filterMenu);
        action->setCheckable(true);
        action->setChecked(checkBox->isChecked());
        checkBox->connect(action, &QAction::toggled, checkBox, &QCheckBox::setChecked); // Keep checkbox and action in sync
        filterMenu->addAction(action);
    };

    addCheckableAction(vertexCheckBox);
    addCheckableAction(edgeCheckBox);
    addCheckableAction(faceCheckBox);
    addCheckableAction(solidCheckBox);

    selectFilterButton->setMenu(filterMenu);
    SARibbonPannelItem::RowProportion rp;
    m_viewPannel->addWidget(selectFilterButton, SARibbonPannelItem::Large);
}

void MainWindow::createToolGroup()
{
    // ---- Tool Group ----
    m_toolCategory = m_ribbon->addCategoryPage(tr("Tool"));

    /* Create Transform Pannel*/
    auto createTransformToolPannel = [&](){
        /* Transform Pannel*/
        m_transformToolPannel = m_toolCategory->addPannel(tr("Transform Tool"));
        // analysisPannel interference
        m_transformAction = new QAction(QIcon(":/icons/icon/tool_transform.svg"), tr("Transform"), this); // Assuming an icon path
        connect(m_transformAction, &QAction::triggered, this, &MainWindow::transform);
        m_transformToolPannel->addLargeAction(m_transformAction);
    };
    createTransformToolPannel();

    /* Create Analysis Pannel*/
    auto createAnalysisPannel = [&](){
        /* Analysis Pannel*/
        m_analysisPannel = m_toolCategory->addPannel(tr("Analysis"));
        // analysisPannel interference
        m_interferenceAction = new QAction(QIcon(":/icons/icon/interference.png"), tr("Interference"), this); // Assuming an icon path
        connect(m_interferenceAction, &QAction::triggered, this, &MainWindow::checkInterference);
        m_analysisPannel->addLargeAction(m_interferenceAction);
    };
    createAnalysisPannel();

    /* Create Clipping Pannel*/
    auto createClippingPannel = [&](){
        /* Clipping Pannel */
        m_clippingPannel = m_toolCategory->addPannel(tr("Clipping Tool"));
        // clipping
        m_clippingAction = new QAction(QIcon(":/icons/icon/clipping.svg"), tr("Clipping"), this); // Assuming an icon path
        connect(m_clippingAction, &QAction::triggered, this, &MainWindow::clipping);
        m_clippingPannel->addLargeAction(m_clippingAction);
    };
    createClippingPannel();

    /* create Measure Pannel */
    auto createMeasurePannel = [&](){
        /* Measure Pannel */
        m_measurePannel = m_toolCategory->addPannel(tr("Measure"));
        // measure distance
        m_measureDistanceAction = new QAction(QIcon(":/icons/icon/measure_distance.svg"), tr("MeasureDistance"), this);
        connect(m_measureDistanceAction, &QAction::triggered, this, &MainWindow::measureDistance);
        m_measurePannel->addSmallAction(m_measureDistanceAction);

        // measure length
        m_measureLengthAction = new QAction(QIcon(":/icons/icon/measure_length.svg"), tr("MeasureLength"), this);
        connect(m_measureLengthAction, &QAction::triggered, this, &MainWindow::measureLength);
        m_measurePannel->addSmallAction(m_measureLengthAction);

        // measure arc length
        m_measureArcLengthAction = new QAction(QIcon(":/icons/icon/measure_arc_length.svg"), tr("MeasureArcLength"), this);
        connect(m_measureArcLengthAction, &QAction::triggered, this, &MainWindow::measureArcLength);
        m_measurePannel->addSmallAction(m_measureArcLengthAction);

        // measure angle
        m_measureAngleAction = new QAction(QIcon(":/icons/icon/measure_angle.svg"), tr("MeasureAngle"), this); 
        connect(m_measureAngleAction, &QAction::triggered, this, &MainWindow::measureAngle);
        m_measurePannel->addSmallAction(m_measureAngleAction);
    };
    createMeasurePannel();

    /* Create Other Pannel */
    auto createOtherPannel = [&](){
        /* Other Pannel */
        m_otherPannel = m_toolCategory->addPannel(tr("Other Tools")); // Adding to Analysis category for simplicity
        // explosion
        m_explosionAction = new QAction(QIcon(":/icons/icon/explosion.png"), tr("Explosion"), this); // Assuming an icon path
        connect(m_explosionAction, &QAction::triggered, this, &MainWindow::explosion);
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
        connect(m_pointAction, &QAction::triggered, this, &MainWindow::createPoint);
        m_shape2dPannel->addSmallAction(m_pointAction);

        // line
        m_lineAction = new QAction(QIcon(":/icons/icon/shape_line.svg"),tr("Line"), this);
        connect(m_lineAction, &QAction::triggered, this, &MainWindow::createLine);
        m_shape2dPannel->addSmallAction(m_lineAction);

        // rectangle
        m_rectangleAction = new QAction(QIcon(":/icons/icon/shape_rectangle.svg"),tr("Rectangle"), this);
        connect(m_rectangleAction, &QAction::triggered, this, &MainWindow::createRectangle);
        m_shape2dPannel->addSmallAction(m_rectangleAction);

        // circle
        m_circleAction = new QAction(QIcon(":/icons/icon/shape_circle.svg"),tr("Circle"), this);
        connect(m_circleAction, &QAction::triggered, this, &MainWindow::createCircle);
        m_shape2dPannel->addSmallAction(m_circleAction);

        // arc
        m_arcAction = new QAction(QIcon(":/icons/icon/shape_arc.svg"),tr("Arc"), this);
        connect(m_arcAction, &QAction::triggered, this, &MainWindow::createArc);
        m_shape2dPannel->addSmallAction(m_arcAction);

        // ellipse
        m_ellipseAction = new QAction(QIcon(":/icons/icon/shape_ellipse.svg"),tr("Ellipse"), this);
        connect(m_ellipseAction, &QAction::triggered, this, &MainWindow::createEllipse);
        m_shape2dPannel->addSmallAction(m_ellipseAction);

        // polygon
        m_polygonAction = new QAction(QIcon(":/icons/icon/shape_polyline.svg"),tr("Polygon"), this);
        connect(m_polygonAction, &QAction::triggered, this, &MainWindow::createPolygon);
        m_shape2dPannel->addSmallAction(m_polygonAction);

        // bezier
        m_bezierCurveAction = new QAction(QIcon(":/icons/icon/shape_bezier.svg"),tr("Bezier"), this);
        connect(m_bezierCurveAction, &QAction::triggered, this, &MainWindow::createBezierCurve);
        m_shape2dPannel->addSmallAction(m_bezierCurveAction);

        // nurbs
        m_nurbsCurveAction = new QAction(QIcon(":/icons/icon/shape_nurbs.svg"),tr("Nurbs"), this);
        connect(m_nurbsCurveAction, &QAction::triggered, this, &MainWindow::createNurbsCurve);
        m_shape2dPannel->addSmallAction(m_nurbsCurveAction);
    };
    createShape2dPannel();

    auto createShape3dPannel = [&](){
        // ---- 3D Shape Pannel ----
        m_shape3dPannel = m_shapeCategory->addPannel(tr("3D"));

        // box
        m_boxAction = new QAction(QIcon(":/icons/icon/box.png"), tr("Box"), this);
        connect(m_boxAction, &QAction::triggered, this, &MainWindow::createBox);
        m_shape3dPannel->addLargeAction(m_boxAction);

        // sphere
        m_sphereAction = new QAction(QIcon(":/icons/icon/sphere.png"), tr("Sphere"), this);
        connect(m_sphereAction, &QAction::triggered, this, &MainWindow::createSphere);
        m_shape3dPannel->addLargeAction(m_sphereAction);

        // cylinder
        m_cylinderAction = new QAction(QIcon(":/icons/icon/cylinder.png"), tr("Cylinder"), this);
        connect(m_cylinderAction, &QAction::triggered, this, &MainWindow::createCylinder);
        m_shape3dPannel->addLargeAction(m_cylinderAction);

        // cone
        m_coneAction = new QAction(QIcon(":/icons/icon/cone.png"), tr("Cone"), this);
        connect(m_coneAction, &QAction::triggered, this, &MainWindow::createCone);
        m_shape3dPannel->addLargeAction(m_coneAction);
    };
    createShape3dPannel();

    /* shape Boolean Pannel Pannel  */
    auto createShapeBooleanPannel = [&](){
        m_shapeBooleanPannel = m_shapeCategory->addPannel(tr("Boolean"));

        // boolean union
        m_booleanUnionAction = new QAction(QIcon(":/icons/icon/boolean_union.svg"), tr("Union"), this);
        connect(m_booleanUnionAction, &QAction::triggered, this, &MainWindow::booleanUnionAction);
        m_shapeBooleanPannel->addLargeAction(m_booleanUnionAction);

        // boolean Intersection
        m_booleanIntersectionAction = new QAction(QIcon(":/icons/icon/boolean_intersection.svg"), tr("Intersection"), this);
        connect(m_booleanIntersectionAction, &QAction::triggered, this, &MainWindow::booleanIntersection);
        m_shapeBooleanPannel->addLargeAction(m_booleanIntersectionAction);

        // boolean difference
        m_booleanDifferenceAction = new QAction(QIcon(":/icons/icon/boolean_difference.svg"), tr("Difference"), this);
        connect(m_booleanDifferenceAction, &QAction::triggered, this, &MainWindow::booleanDifference);
        m_shapeBooleanPannel->addLargeAction(m_booleanDifferenceAction);
    };
    createShapeBooleanPannel();

}

void MainWindow::createHelpGroup()
{
    // ---- help Group ----
    m_helpCategory = m_ribbon->addCategoryPage(tr("Help"));
    // ---- help ----
    // These actions can be added to an existing pannel or a new one
    m_versionPannel = m_helpCategory->addPannel( tr("Version") );  // Adding to Analysis category for simplicity
    m_versionAction = new QAction(QIcon(":/icons/icon/version.svg"), tr("Version"), this);  // Assuming an icon path
    connect(m_versionAction, &QAction::triggered, this, &MainWindow::version);
    m_versionPannel->addLargeAction(m_versionAction);

    m_languagePannel = m_helpCategory->addPannel(tr("Language"));
    m_languageAction = new QAction(QIcon(":/icons/icon/help_language.svg"), tr("Switch Language"), this);
    connect(m_languageAction, &QAction::triggered, this, &MainWindow::switchLanguage);
    m_languagePannel->addLargeAction(m_languageAction);
}

void MainWindow::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open CAD File", "", "STEP (*.step *.stp);;IGES (*.iges *.igs)");
    if (!filename.isEmpty())
    {
        m_viewerWidget->loadModel(filename);
    }
}

void MainWindow::viewFit() const
{
    m_viewerWidget->viewFit();
}

void MainWindow::onSelectModeToggled(bool checked)
{
    //m_viewerWidget->setFilters(checked);
}

void MainWindow::onSelectFilter(int index) const
{
}

void MainWindow::checkInterference() const
{
    m_viewerWidget->checkInterference();
}

void MainWindow::transform() const
{
    m_viewerWidget->transform();
}

void MainWindow::clipping() const
{
    const gp_Dir normal(0.0, 0.0, 1.0);
    const gp_Pnt point(0.0, 0.0, 10.0);
    m_viewerWidget->clipping( normal , point );
}

void MainWindow::explosion()
{
    if(!m_widgetExplodeAsm)
        return;
    m_widgetExplodeAsm->show();
    m_widgetExplodeAsm->adjustSize();
}

void MainWindow::measureDistance() const
{
    m_viewerWidget->measureDistance();
}

void MainWindow::measureLength() const
{
    // TODO
}

void MainWindow::measureArcLength() const
{
    // TODO
}

void MainWindow::measureAngle() const
{
    // TODO
}

void MainWindow::version()
{
    DialogAbout dlg(this);
    dlg.exec();
}

void MainWindow::createPoint()
{
    gp_Pnt p(10, 20, 30);
    BRepBuilderAPI_MakeVertex vertexMaker(p);
    if (vertexMaker.IsDone())
    {
        m_viewerWidget->displayShape(vertexMaker.Shape(), 1.0, 1.0, 1.0); 
    }
}

void MainWindow::createLine()
{
    gp_Pnt p1(0, 0, 0);
    gp_Pnt p2(50, 50, 50);
    BRepBuilderAPI_MakeEdge edge(p1, p2);
    if (edge.IsDone())
    {
        m_viewerWidget->displayShape(edge.Shape(), 1.0, 0.0, 0.0);
    }
}

void MainWindow::createRectangle()
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

void MainWindow::createCircle()
{
    gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); // Z axis
    gp_Circ circle(axis, 25.0);                   // Radius 25
    BRepBuilderAPI_MakeEdge edge(circle);
    if (edge.IsDone())
    {
        m_viewerWidget->displayShape(edge.Shape(), 0.0, 0.0, 1.0);
    }
}

void MainWindow::createArc()
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

void MainWindow::createEllipse()
{
    gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)); // Z axis
    gp_Elips ellipse(axis, 30.0, 15.0);           // Major radius 30, minor 15
    BRepBuilderAPI_MakeEdge edge(ellipse);
    if (edge.IsDone())
    {
        m_viewerWidget->displayShape(edge.Shape(), 1.0, 0.0, 1.0);
    }
}

void MainWindow::createPolygon()
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

void MainWindow::createBezierCurve()
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

void MainWindow::createNurbsCurve()
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

void MainWindow::createBox()
{
    BRepPrimAPI_MakeBox box(10.0, 10.0, 10.0);
    m_viewerWidget->displayShape(box.Shape(), 1.0, 0.0, 1.0);
}

void MainWindow::createPyramid()
{
    m_viewerWidget->createPyramid();
}

void MainWindow::createSphere()
{
    BRepPrimAPI_MakeSphere sphere( gp_Pnt( 0, 0, 0 ), 5.0 ) ;
    m_viewerWidget->displayShape( sphere.Shape(), 1.0, 0.0, 0.0 ) ;  // NOLINT
}

void MainWindow::createCylinder()
{
    BRepPrimAPI_MakeCylinder cylinder(5.0, 10.0);
    m_viewerWidget->displayShape(cylinder.Shape(), 0.0, 1.0, 0.0);
}

void MainWindow::createCone()
{
    BRepPrimAPI_MakeCone cone(5.0, 0.0, 10.0);
    m_viewerWidget->displayShape(cone.Shape(), 0.0, 1.0, 1.0);
}

void MainWindow::booleanUnionAction()
{
    m_viewerWidget->booleanUnion();
}

void MainWindow::booleanIntersection()
{
    m_viewerWidget->booleanIntersection();
}

void MainWindow::booleanDifference()
{
    m_viewerWidget->booleanDifference();
}

ViewerWidget* MainWindow::GetViewerWidget() const
{
    return m_viewerWidget;
}

ModelTreeWidget* MainWindow::GetModelTreeWidget() const
{
    return m_modelTreeWidget;
}

void MainWindow::switchLanguage()
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
