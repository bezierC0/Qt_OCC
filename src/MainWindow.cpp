#include "MainWindow.h"
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QFileDialog>
#include <QAction>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    m_viewerWidget = new ViewerWidget(this);
    setCentralWidget(m_viewerWidget);
    createToolBar();

    resize(800, 600);
}

void MainWindow::createToolBar()
{
    QToolBar* toolbar = addToolBar("Main Toolbar");

    // ---- File Group ----
    QToolButton* fileButton = new QToolButton();
    fileButton->setText( "File" );
    fileButton->setPopupMode( QToolButton::InstantPopup );
    QMenu* fileMenu = new QMenu( fileButton );
    QAction* openAct = fileMenu->addAction( "Open" );
    connect( openAct, &QAction::triggered, this, &MainWindow::openFile );
    fileButton->setMenu( fileMenu );
    toolbar->addWidget( fileButton );

    // ---- View Group ----
    QToolButton* viewButton = new QToolButton();
    viewButton->setText( "View" );
    viewButton->setPopupMode( QToolButton::InstantPopup );

    QMenu* viewMenu = new QMenu( viewButton );
    QAction* fitAct = viewMenu->addAction( "Fit" );
    connect( fitAct, &QAction::triggered, this, &MainWindow::viewFit );

    viewButton->setMenu( viewMenu );
    toolbar->addWidget( viewButton );

    // ---- Analysis Group ----
    QToolButton* analysisButton = new QToolButton();
    analysisButton->setText( "Analysis" );
    analysisButton->setPopupMode( QToolButton::InstantPopup );

    QMenu* analysisMenu = new QMenu( analysisButton );
    QAction* interferenceAct = analysisMenu->addAction( "Interference" );
    connect( interferenceAct, &QAction::triggered, this, &MainWindow::checkInterference );

    analysisButton->setMenu( analysisMenu );
    toolbar->addWidget( analysisButton );

    // ---- Others (flat actions) ----

    const auto clippingAct = toolbar->addAction("clipping");
    connect(clippingAct, &QAction::triggered, this, &MainWindow::clipping);

    const auto explosionAct = toolbar->addAction("explosion");
    connect(explosionAct, &QAction::triggered, this, &MainWindow::explosion);
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

void MainWindow::clipping() const
{
    const gp_Dir normal(0.0,0.0,1.0);
    const gp_Pnt point( 0.0, 0.0, 10.0 );
    m_viewerWidget->clipping( normal , point );
}

void MainWindow::explosion() const
{
    m_viewerWidget->explosion();
}
