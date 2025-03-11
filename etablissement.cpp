#include "etablissement.h"
#include <string>
#include <QSqlQuery>
#include <QDebug>
#include <QSqlError>
#include <QSqlTableModel>
#include <QTableView>
#include <QSqlQueryModel>

Etablissement::Etablissement(int id_etab, std::string nom, std::string gouvernorat, float longe, float lat, int capacite, std::string email, int tel)
{
    this->id_etab = id_etab;
    this->nom = nom;
    this->gouvernorat = gouvernorat;
    this->longe = longe;
    this->lat = lat;
    this->capacite = capacite;
    this->email = email;
    this->tel = tel;
}

// Getters

int Etablissement::getID()
{
    return id_etab;
}

std::string Etablissement::getNom()
{
    return nom;
}

std::string Etablissement::getGouv()
{
    return gouvernorat;
}

float Etablissement::getLonge()
{
    return longe;
}

float Etablissement::getLat()
{
    return lat;
}

int Etablissement::getCap()
{
    return capacite;
}

std::string Etablissement::getEmail()
{
    return email;
}

int Etablissement::getTel()
{
    return tel;
}

// Setters
void Etablissement::setNom(std::string nom)
{
    this->nom = nom;
}

void Etablissement::setGouv(std::string gouvernorat)
{
    this->gouvernorat = gouvernorat;
}

void Etablissement::setLonge(float longe)
{
    this->longe = longe;
}

void Etablissement::setLat(float lat)
{
    this->lat = lat;
}

void Etablissement::setCap(int capacite)
{
    this->capacite = capacite;
}

void Etablissement::setEmail(std::string email)
{
    this->email = email;
}

void Etablissement::setTel(int tel)
{
    this->tel = tel;
}


bool Etablissement::ajouter()
{
    QSqlQuery query;
    query.prepare("INSERT INTO ETABLISSEMENTS (NOM, GOUVERNORAT, LONGE, LAT, CAPACITE, MAIL, TEL) "
                  "VALUES (:nom, :gouvernorat, :longe, :lat, :capacite, :email, :tel)");

    query.bindValue(":nom", QString::fromStdString(nom));
    query.bindValue(":gouvernorat", QString::fromStdString(gouvernorat));
    query.bindValue(":longe", longe);
    query.bindValue(":lat", lat);
    query.bindValue(":capacite", capacite);
    query.bindValue(":email", QString::fromStdString(email));
    query.bindValue(":tel", tel);

    if (!query.exec()) {
        qDebug() << "Erreur d'insertion:" << query.lastError().text();
        return false;
    }

    return true;
}


void Etablissement::afficher(QTableView *tableView)
{
    model = new QSqlQueryModel();

    model->setQuery("SELECT * FROM ETABLISSEMENTS");

    if (model->lastError().isValid()) {
        qDebug() << "Erreur lors de l'exécution de la requête :" << model->lastError().text();
        return;
    }

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Gouvernorat"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Longitude"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Latitude"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Capacité"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Téléphone"));

    tableView->setModel(model);
}



bool Etablissement::supprimerTous()
{

    QSqlQuery query;
    query.prepare("DELETE FROM ETABLISSEMENTS");

    if (!query.exec()) {
        qDebug() << "Erreur lors de la suppression:" << query.lastError().text();
        return false;
    }

    return true;
}


bool Etablissement::supprimer(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM ETABLISSEMENTS WHERE ID_ETAB = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur lors de la suppression:" << query.lastError().text();
        return false;
    }

    return true;
}

bool Etablissement::modifier(int id)
{
    QSqlQuery query;
    query.prepare("UPDATE ETABLISSEMENTS SET NOM = :nom, GOUVERNORAT = :gouvernorat, "
                  "LONGE = :longe, LAT = :lat, CAPACITE = :capacite, "
                  "MAIL = :email, TEL = :tel WHERE ID_ETAB = :id");

    query.bindValue(":nom", QString::fromStdString(nom));
    query.bindValue(":gouvernorat", QString::fromStdString(gouvernorat));
    query.bindValue(":longe", longe);
    query.bindValue(":lat", lat);
    query.bindValue(":capacite", capacite);
    query.bindValue(":email", QString::fromStdString(email));
    query.bindValue(":tel", tel);
    query.bindValue(":id", id);
    if (!query.exec()) {
        qDebug() << "Erreur lors de la modification:" << query.lastError().text();
        return false;
    }
    return true;
}

