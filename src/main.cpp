#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // QApplication manages the application's event loop and settings.
    QApplication a(argc, argv);

    a.setWindowIcon(QIcon(":/resources/icon.png"));

    // Create and show your main window.
    MainWIndow w;
    w.show();

    // Start the application's event loop.
    return a.exec();
}
