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
    
    if ( m_ribbon )
    {
        m_ribbon->clear();
        m_ribbon->removeCategory( m_fileCategory );
        m_ribbon->removeCategory( m_viewCategory );
        m_ribbon->removeCategory( m_analysisCategory );
        m_ribbon->removeCategory( m_shapeCategory );
        m_ribbon->removeCategory( m_helpCategory );

    }
    else
    {
        m_ribbon = new SARibbonBar(this);
        setRibbonBar( m_ribbon );
    }
    
    m_ribbon->setRibbonStyle(SARibbonBar::RibbonStyleLooseThreeRow); // Or other styles like WpsLiteStyle

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

    // ---- Tool Group ----
    m_analysisCategory = m_ribbon->addCategoryPage(tr("Tool"));
    
    /* Analysis Pannel*/
    m_analysisPannel = m_analysisCategory->addPannel(tr("Analysis"));
    // analysisPannel interference
    m_interferenceAction = new QAction(QIcon(":/icons/icon/interference.png"), tr("Interference"), this); // Assuming an icon path
    connect(m_interferenceAction, &QAction::triggered, this, &MainWindow::checkInterference);
    m_analysisPannel->addLargeAction(m_interferenceAction);

    /* Clipping Pannel */
    m_clippingPannel = m_analysisCategory->addPannel(tr("Clipping Tool"));
    // clipping
    m_clippingAction = new QAction(QIcon(":/icons/icon/clipping.svg"), tr("Clipping"), this); // Assuming an icon path
    connect(m_clippingAction, &QAction::triggered, this, &MainWindow::clipping);
    m_clippingPannel->addSmallAction(m_clippingAction);

    /* Measure Pannel */
    m_measurePannel = m_analysisCategory->addPannel(tr("Measure"));
    // measure distance
    m_measureDistanceAction = new QAction(QIcon(":/icons/icon/explosion.png"), tr("MeasureDistance"), this); // Assuming an icon path
    connect(m_measureDistanceAction, &QAction::triggered, this, &MainWindow::measureDistance);
    m_measurePannel->addSmallAction(m_measureDistanceAction);
    
    /* Other Pannel */
    m_otherPannel = m_analysisCategory->addPannel(tr("Other Tools")); // Adding to Analysis category for simplicity
    // explosion
    m_explosionAction = new QAction(QIcon(":/icons/icon/explosion.png"), tr("Explosion"), this); // Assuming an icon path
    connect(m_explosionAction, &QAction::triggered, this, &MainWindow::explosion);
    m_otherPannel->addSmallAction(m_explosionAction);

    // ---- Shape Group ----
    m_shapeCategory = m_ribbon->addCategoryPage(tr("Shape"));
    m_basicShapesPannel = m_shapeCategory->addPannel(tr("Basic Shapes"));

    // box
    m_boxAction = new QAction(QIcon(":/icons/icon/box.png"), tr("Box"), this);
    connect(m_boxAction, &QAction::triggered, this, &MainWindow::createBox);
    m_basicShapesPannel->addLargeAction(m_boxAction);

    // sphere
    m_sphereAction = new QAction(QIcon(":/icons/icon/sphere.png"), tr("Sphere"), this);
    connect(m_sphereAction, &QAction::triggered, this, &MainWindow::createSphere);
    m_basicShapesPannel->addLargeAction(m_sphereAction);

    // cylinder
    m_cylinderAction = new QAction(QIcon(":/icons/icon/cylinder.png"), tr("Cylinder"), this);
    connect(m_cylinderAction, &QAction::triggered, this, &MainWindow::createCylinder);
    m_basicShapesPannel->addLargeAction(m_cylinderAction);

    m_coneAction = new QAction(QIcon(":/icons/icon/cone.png"), tr("Cone"), this);
    connect(m_coneAction, &QAction::triggered, this, &MainWindow::createCone);
    m_basicShapesPannel->addLargeAction(m_coneAction);

    // ---- help Group ----
    m_helpCategory = m_ribbon->addCategoryPage(tr("Help"));
    // ---- help ----
    // These actions can be added to an existing pannel or a new one
    m_versionPannel = m_helpCategory->addPannel( tr("Version") );  // Adding to Analysis category for simplicity
    m_versionAction = new QAction(QIcon(":/icons/icon/version.svg"), tr("Version"), this);  // Assuming an icon path
    connect(m_versionAction, &QAction::triggered, this, &MainWindow::version);
    m_versionPannel->addSmallAction(m_versionAction);

    m_languagePannel = m_helpCategory->addPannel(tr("Language"));
    m_languageAction = new QAction(QIcon(), tr("Switch Language"), this);
    connect(m_languageAction, &QAction::triggered, this, &MainWindow::switchLanguage);
    m_languagePannel->addSmallAction(m_languageAction);
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

void MainWindow::measureDistance() const
{
    m_viewerWidget->measureDistance();
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
    m_viewerWidget->setFilters(checked);
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
    if (m_translator->load(QString(":/language/i18n/qt_occ_") + langCode))
    {
        QCoreApplication::installTranslator( m_translator );
    }
    
    // Re-create the UI to apply the new language
    setupUi();
}
