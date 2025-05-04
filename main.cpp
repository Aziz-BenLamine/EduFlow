#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include "connection.h"

//gg
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Connection c;
    bool test = c.createconnect();
    MainWindow w;
    if (test) {
        w.show();
        QMessageBox::information(nullptr, QObject::tr("database is open"),
                                 QObject::tr("connection successful.\n"
                                             "Click Cancel to exit."), QMessageBox::Cancel);
    } else {
        QMessageBox::critical(nullptr, QObject::tr("database error"),
                              QObject::tr("connection failed.\n"
                                          "Click Cancel to exit."), QMessageBox::Cancel);
        return 1; // Exit with error code if connection fails
    }

    return a.exec();
}
