#ifndef CONNECTION_H
#define CONNECTION_H
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

class Connection {
public:
    Connection();
    bool createconnect();
    void closeConnection();
    bool isOpen() const;
private:
    QSqlDatabase db; // Store the database object for connection management
};

#endif // CONNECTION_H
