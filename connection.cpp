#include "connection.h"
#include <QDebug>

Connection::Connection()
{
    db = QSqlDatabase::addDatabase("QODBC", "EduFlowConnection"); // Named connection
}

Connection::~Connection()
{
    closeConnection();
}

bool Connection::createconnect()
{
    bool test = false;
    db.setDatabaseName("EduFlow"); // Ensure this matches your ODBC DSN
    db.setUserName("Deepsight");
    db.setPassword("123");

    if (db.open()) {
        qDebug() << "Database opened successfully.";
        test = true;
    } else {
        qDebug() << "Failed to open database:" << db.lastError().text();
    }
    return test;
}

void Connection::closeConnection()
{
    if (db.isOpen()) {
        db.close();
        qDebug() << "Database connection closed.";
    }
    QSqlDatabase::removeDatabase("EduFlowConnection"); // Clean up named connection
}
