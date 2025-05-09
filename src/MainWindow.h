#pragma once

#include <QMainWindow>

class ViewerWidget;
class ModelTreeWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ViewerWidget* GetViewerWidget() const;
    ModelTreeWidget* GetModelTreeWidget() const;
private slots:
    void openFile();
    void viewFit() const;
    void checkInterference() const;
    void clipping() const;
    void explosion() const;

private:
    void createToolBar();

private:
    ViewerWidget* m_viewerWidget;
    ModelTreeWidget* m_modelTreeWidget;
};
