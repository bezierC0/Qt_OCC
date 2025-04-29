#include "MainWindow.h"
#include <QToolBar>
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

    QAction* openAct = toolbar->addAction("Open");
    connect(openAct, &QAction::triggered, this, &MainWindow::openFile);

    QAction* topViewAct = toolbar->addAction("Top View");
    connect(topViewAct, &QAction::triggered, this, &MainWindow::setViewTop);

    QAction* interferenceAct = toolbar->addAction("Interference");
    connect(interferenceAct, &QAction::triggered, this, &MainWindow::checkInterference);

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

void MainWindow::setViewTop() const
{
    m_viewerWidget->setTopView();
}

void MainWindow::checkInterference() const
{
    m_viewerWidget->checkInterference();
}

void MainWindow::clipping() const
{
    m_viewerWidget->clipping();
}

void MainWindow::explosion() const
{
    m_viewerWidget->explosion();
}
