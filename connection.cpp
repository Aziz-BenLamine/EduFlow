#include "connection.h"
#include <QDebug>

Connection::Connection() {
    db = QSqlDatabase::addDatabase("QODBC");
    db.setDatabaseName("EduFlow"); // Name of the ODBC data source
    db.setUserName("Deepsight");   // Oracle username
    db.setPassword("123");         // Oracle password
}

bool Connection::createconnect() {
    bool test = false;
    if (!db.isOpen()) {
        if (db.open()) {
            QSqlQuery query(db);
            // Set NLS_DATE_FORMAT for the session (from gestion_colis)
            if (query.exec("ALTER SESSION SET NLS_DATE_FORMAT = 'YYYY-MM-DD'")) {
                qDebug() << "✅ NLS_DATE_FORMAT set to YYYY-MM-DD";
            } else {
                qDebug() << "❌ Failed to set NLS_DATE_FORMAT:" << query.lastError().text();
            }
            // Verify table accessibility (from gestion_colis)
            if (query.exec("SELECT 1 FROM COLIS WHERE ROWNUM = 1")) {
                qDebug() << "✅ Connexion réussie à la base de données EduFlow!";
                test = true;
            } else {
                qDebug() << "❌ Erreur accès table COLIS:" << query.lastError().text();
            }
        } else {
            qDebug() << "❌ Erreur de connexion:" << db.lastError().text();
        }
    } else {
        test = true; // Already open
    }
    return test;
}

void Connection::closeConnection() {
    if (db.isOpen()) {
        db.close();
    }
    qDebug() << "✅ Connexion à la base de données fermée.";
}

bool Connection::isOpen() const {
    return db.isOpen();
}
