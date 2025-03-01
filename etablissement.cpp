#include "etablissement.h"
#include <string>
#include <QSqlQuery>
#include <QDebug>
#include <QSqlError>
#include <QTableWidget>
#include <QTableWidgetItem>

Etablissement::Etablissement(std::string nom, std::string gouvernorat, float longe, float lat, int capacite, std::string email, int tel)
{
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

Etablissement::Etablissement()
{
    this->nom = "";
    this->gouvernorat = "";
    this->longe = 0.0f;
    this->lat = 0.0f;
    this->capacite = 0;
    this->email = "";
    this->tel = 0;
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


void Etablissement::affichier(QTableWidget * table)
{
    {
        QSqlQuery query;
        query.prepare("SELECT * FROM ETABLISSEMENTS");

        if (!query.exec()) {
            qDebug() << "Erreur lors de la récupération des données:" << query.lastError().text();
            return;
        }

        table->setRowCount(0);
        int row = 0;

        while (query.next()) {
            table->insertRow(row);

            table->setItem(row, 0, new QTableWidgetItem(query.value("ID_ETAB").toString()));
            table->setItem(row, 1, new QTableWidgetItem(query.value("NOM").toString()));
            table->setItem(row, 2, new QTableWidgetItem(query.value("GOUVERNORAT").toString()));
            table->setItem(row, 3, new QTableWidgetItem(query.value("LONGE").toString()));
            table->setItem(row, 4, new QTableWidgetItem(query.value("LAT").toString()));
            table->setItem(row, 5, new QTableWidgetItem(query.value("CAPACITE").toString()));
            table->setItem(row, 6, new QTableWidgetItem(query.value("MAIL").toString()));
            table->setItem(row, 7, new QTableWidgetItem(query.value("TEL").toString()));

            row++;
        }
    }

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


