#include "etablissement.h"
#include <string>
#include <QSqlQuery>
#include <QDebug>
#include <QSqlError>
#include <QSqlTableModel>
#include <QTableView>
#include <QSqlQueryModel>
#include <QHeaderView>

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

void Etablissement::afficher(QTableView* tableView) {
    QSqlQueryModel* model = new QSqlQueryModel();
    model->setQuery("SELECT ID_ETAB, NOM, GOUVERNORAT, LONGE, LAT, CAPACITE, MAIL, TEL FROM ETABLISSEMENTS");

    if (model->lastError().isValid()) {
        qDebug() << "Erreur lors de l'exécution de la requête :" << model->lastError().text();
        return;
    }

    // Vérifier si des lignes sont retournées
    qDebug() << "Nombre de lignes retournées :" << model->rowCount();
    if (model->rowCount() == 0) {
        qDebug() << "Aucune donnée à afficher : la table est vide.";
    }

    // Définir les en-têtes
    model->setHeaderData(0, Qt::Horizontal, QString("ID"));
    model->setHeaderData(1, Qt::Horizontal, QString("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QString("Gouvernorat"));
    model->setHeaderData(3, Qt::Horizontal, QString("Longitude"));
    model->setHeaderData(4, Qt::Horizontal, QString("Latitude"));
    model->setHeaderData(5, Qt::Horizontal, QString("Capacité"));
    model->setHeaderData(6, Qt::Horizontal, QString("Email"));
    model->setHeaderData(7, Qt::Horizontal, QString("Téléphone"));

    // Assigner le modèle au QTableView
    tableView->setModel(model);

    // Ajuster les colonnes
    QHeaderView* header = tableView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);

    // Forcer un redimensionnement explicite pour s'assurer que tout est visible
    tableView->resizeColumnsToContents();

    // Optionnel : Définir une largeur minimale pour la colonne ID si nécessaire
    tableView->setColumnWidth(0, 60); // Ajustez la valeur selon vos besoins

    // Optionnel : Activer le redimensionnement interactif pour permettre à l'utilisateur d'ajuster manuellement
    header->setSectionResizeMode(QHeaderView::Interactive);

    // Assurer que le texte ne soit pas tronqué
    tableView->setWordWrap(false);
    tableView->setTextElideMode(Qt::ElideNone);

    // Afficher le QTableView
    tableView->show();
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

