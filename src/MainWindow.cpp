#include "MainWindow.h"
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QMenu>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QAction>
#include <TopoDS_Shape.hxx> // This should be resolved by CMakeLists.txt fix
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include "ViewerWidget.h"
#include "WidgetModelTree.h"
#include "DialogAbout.h"


MainWindow::MainWindow(QWidget* parent) : SARibbonMainWindow(parent)
{
    m_modelTreeWidget = new ModelTreeWidget( this );
    m_viewerWidget = new ViewerWidget(this);

    const auto splitter = new QSplitter( Qt::Horizontal, this );
    splitter->addWidget( m_modelTreeWidget );
    splitter->addWidget( m_viewerWidget );

    splitter->setStretchFactor( 0, 1 ); // m_modelTreeWidget
    splitter->setStretchFactor( 1, 4 ); 

    setCentralWidget( splitter );

    // Initialize Ribbon Bar
    SARibbonBar* ribbon = ribbonBar(); // Get the ribbon bar from SARibbonMainWindow
    ribbon->setRibbonStyle(SARibbonBar::RibbonStyleLooseThreeRow); // Or other styles like WpsLiteStyle

    // ---- File Group ----
    SARibbonCategory* fileCategory = ribbon->addCategoryPage(QStringLiteral("File"));
    SARibbonPannel* filePannel = fileCategory->addPannel(QStringLiteral("File Operations"));

    // new
    QAction* fileNewAct = new QAction(QIcon(":/icons/icon/file_new.svg"), QStringLiteral("New"), this); // Assuming an icon path
    //connect(fileNewAct, &QAction::triggered, this, &MainWindow::newFile);
    filePannel->addLargeAction(fileNewAct);

    // open
    QAction* openAct = new QAction(QIcon(":/icons/icon/open.png"), QStringLiteral("Open"), this); // Assuming an icon path
    connect(openAct, &QAction::triggered, this, &MainWindow::openFile);
    filePannel->addLargeAction(openAct);

    // ---- View Group ----
    SARibbonCategory* viewCategory = ribbon->addCategoryPage(QStringLiteral("View"));
    SARibbonPannel* viewPannel = viewCategory->addPannel(QStringLiteral("View Operations"));
    QAction* fitAct = new QAction(QIcon(":/icons/icon/fit.png"), QStringLiteral("Fit"), this); // Assuming an icon path
    connect(fitAct, &QAction::triggered, this, &MainWindow::viewFit);
    viewPannel->addLargeAction(fitAct);

    // select
    QAction* selectAction = new QAction(QIcon(":/icons/icon/select.svg"), QStringLiteral("Select"), this);
    selectAction->setCheckable(true);
    connect(selectAction, &QAction::toggled, this, &MainWindow::onSelectModeToggled);
    viewPannel->addLargeAction(selectAction);

    // ---- Analysis Group ----
    SARibbonCategory* analysisCategory = ribbon->addCategoryPage(QStringLiteral("Analysis"));
    SARibbonPannel* analysisPannel = analysisCategory->addPannel(QStringLiteral("Analysis Tools"));
    QAction* interferenceAct = new QAction(QIcon(":/icons/icon/interference.png"), QStringLiteral("Interference"), this); // Assuming an icon path
    connect(interferenceAct, &QAction::triggered, this, &MainWindow::checkInterference);
    analysisPannel->addLargeAction(interferenceAct);

    // ---- Shape Group ----
    SARibbonCategory* shapeCategory = ribbon->addCategoryPage(QStringLiteral("Shape"));
    SARibbonPannel* basicShapesPannel = shapeCategory->addPannel(QStringLiteral("Basic Shapes"));

    // box
    QAction* boxAct = new QAction(QIcon(":/icons/icon/box.png"), QStringLiteral("Box"), this);
    connect(boxAct, &QAction::triggered, this, &MainWindow::createBox);
    basicShapesPannel->addLargeAction(boxAct);

    // sphere
    QAction* sphereAct = new QAction(QIcon(":/icons/icon/sphere.png"), QStringLiteral("Sphere"), this);
    connect(sphereAct, &QAction::triggered, this, &MainWindow::createSphere);
    basicShapesPannel->addLargeAction(sphereAct);

    // cylinder
    QAction* cylinderAct = new QAction(QIcon(":/icons/icon/cylinder.png"), QStringLiteral("Cylinder"), this);
    connect(cylinderAct, &QAction::triggered, this, &MainWindow::createCylinder);
    basicShapesPannel->addLargeAction(cylinderAct);

    QAction* coneAct = new QAction(QIcon(":/icons/icon/cone.png"), QStringLiteral("Cone"), this);
    connect(coneAct, &QAction::triggered, this, &MainWindow::createCone);
    basicShapesPannel->addLargeAction(coneAct);

    // ---- Others (flat actions) ----
    // These actions can be added to an existing pannel or a new one
    SARibbonPannel* otherPannel = analysisCategory->addPannel(QStringLiteral("Other Tools")); // Adding to Analysis category for simplicity
    QAction* clippingAct = new QAction(QIcon(":/icons/icon/clipping.svg"), QStringLiteral("Clipping"), this); // Assuming an icon path
    connect(clippingAct, &QAction::triggered, this, &MainWindow::clipping);
    otherPannel->addSmallAction(clippingAct);

    QAction* explosionAct = new QAction(QIcon(":/icons/icon/explosion.png"), QStringLiteral("Explosion"), this); // Assuming an icon path
    connect(explosionAct, &QAction::triggered, this, &MainWindow::explosion);
    otherPannel->addSmallAction(explosionAct);

    // ---- help Group ----
    SARibbonCategory* helpCategory = ribbon->addCategoryPage(QStringLiteral("Help"));
    // ---- help ----
    // These actions can be added to an existing pannel or a new one
    SARibbonPannel* versionPannel = helpCategory->addPannel( QStringLiteral("Version") );  // Adding to Analysis category for simplicity
    QAction* versionAct = new QAction(QIcon(":/icons/icon/version.svg"), QStringLiteral("Version"), this);  // Assuming an icon path
    connect(versionAct, &QAction::triggered, this, &MainWindow::version);
    versionPannel->addSmallAction(versionAct);


    resize(800, 600);
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

void MainWindow::explosion() const
{
    m_viewerWidget->explosion();
}

void MainWindow::version()
{
    DialogAbout dlg(this);
    dlg.exec();
}

void MainWindow::createBox()
{
    BRepPrimAPI_MakeBox box(10.0, 10.0, 10.0);
    m_viewerWidget->displayShape(box.Shape(), 1.0, 0.0, 1.0);
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

ViewerWidget* MainWindow::GetViewerWidget() const
{
    return m_viewerWidget;
}

ModelTreeWidget* MainWindow::GetModelTreeWidget() const
{
    return m_modelTreeWidget;
}

void MainWindow::onSelectModeToggled(bool checked)
{
    if (checked)
    {
        m_viewerWidget->setInteractionMode(InteractionMode::Select);
    }
    else
    {
        m_viewerWidget->setInteractionMode(InteractionMode::Highlight);
    }
}
