#include "MainWindow.h"
#include <QApplication>
#include <QSplashScreen>
#include <QPointer>

int main( int argc, char *argv[] )
{
    QApplication app( argc, argv ) ;

    QPointer<QSplashScreen> splash = new QSplashScreen() ;
    splash->setPixmap( QPixmap( ":/images/images/logo.png" ) ) ;
    splash->show() ;
    QApplication::processEvents() ;

    MainWindow w ;
    w.show() ;
    splash->finish( &w ) ;
    return QApplication::exec() ;
}
