#include "mainwindow.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QScreen>
#include <QtGlobal>

int main(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication a(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("equipements.db");
    if (!db.open()) {
        qDebug() << "Erreur de connexion à la base :" << db.lastError().text();
        return -1;
    }

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS EQUIPEMENTS ("
               "ID_EQ INTEGER PRIMARY KEY, "
               "NOM_EQ TEXT NOT NULL, "
               "TYPEEQ TEXT NOT NULL, "
               "ETATEQ TEXT NOT NULL, "
               "MARQUEEQ TEXT NOT NULL, "
               "QT INTEGER NOT NULL, "
               "DATEEQ TEXT NOT NULL, "
               "IMAGE_EQ BLOB)");
    if (!query.isActive()) {
        qDebug() << "Erreur lors de la création de la table :" << query.lastError().text();
    }

    MainWindow w;

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    qDebug() << "Résolution de l'écran détectée :" << screenGeometry.width() << "x" << screenGeometry.height();

    int width = qMin(static_cast<int>(screenGeometry.width() * 0.5), 800);
    int height = qMin(static_cast<int>(screenGeometry.height() * 0.5), 450);

    w.resize(width, height);
    w.move((screenGeometry.width() - width) / 2, (screenGeometry.height() - height) / 2);

    w.show();
    return a.exec();
}
