#include "mainwindow.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Configuration de la base de données SQLite
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("equipements.db");
    if (!db.open()) {
        qDebug() << "Erreur de connexion à la base :" << db.lastError().text();
        return -1;
    }

    // Création de la table si elle n'existe pas
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
    w.show();
    return a.exec();
}
