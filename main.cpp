#include "mainwindow.h"
#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QScreen>
#include <QtGlobal> // Pour qMin

int main(int argc, char *argv[]) {
    // Activer le support HiDPI pour les écrans à haute densité
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

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

    // Ajuster la taille de la fenêtre à l'écran
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    qDebug() << "Résolution de l'écran détectée :" << screenGeometry.width() << "x" << screenGeometry.height();

    // Définir une taille de fenêtre adaptée (50% de l'écran, avec max pour écrans petits)
    int width = qMin(static_cast<int>(screenGeometry.width() * 0.5), 800);  // Réduit à 50% et limite à 800
    int height = qMin(static_cast<int>(screenGeometry.height() * 0.5), 450); // Réduit à 50% et limite à 450

    w.resize(width, height);

    // Centrer la fenêtre
    w.move((screenGeometry.width() - width) / 2, (screenGeometry.height() - height) / 2);

    w.show();
    return a.exec();
}
