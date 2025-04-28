#pragma once

#include <QMainWindow>
#include "ViewerWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
private slots:
    void openFile();
    void setViewTop() const;
    void checkInterference() const;

private:
    ViewerWidget* m_viewerWidget;
    void createToolBar();
};
