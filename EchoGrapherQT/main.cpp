#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setWindowIcon(QIcon(":/assets/appicon.png"));
    a.setStyleSheet("QWidget { background-color: #232633; }");
    MainWindow w;
    w.show();
    return a.exec();
}
