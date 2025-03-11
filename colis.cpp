#include "colis.h"
#include <QDebug>
#include <QSqlError>

Colis::Colis()
    : idColis(0), idEmploye(0), idEtab(0), capacite(0), dateArrivee(QDate()), dateSortie(QDate()), statut("")
{
}

Colis::Colis(int idColis, int idEmploye, int idEtab, int capacite, QDate dateArrivee, QDate dateSortie, QString statut)
    : idColis(idColis), idEmploye(idEmploye), idEtab(idEtab), capacite(capacite),
    dateArrivee(dateArrivee), dateSortie(dateSortie), statut(statut)
{
}

bool Colis::ajouter()
{
    QSqlQuery query(QSqlDatabase::database("EduFlowConnection"));
    query.prepare("INSERT INTO COLIS (ID_COLIS, STATUT, DATE_SORTIE, DATE_ARRIVEE_ESTIMEE, CAPACITE, ID_ETAB, ID_EMPLOYE) "
                  "VALUES (:id_colis, :statut, :date_sortie, :date_arrivee_estimee, :capacite, :id_etab, :id_employe)");
    query.bindValue(":id_colis", idColis);
    query.bindValue(":statut", statut);
    query.bindValue(":date_sortie", dateSortie); // No need for isValid() check here; validation is done upstream
    query.bindValue(":date_arrivee_estimee", dateArrivee.isValid() ? QVariant(dateArrivee) : QVariant()); // NULL if invalid
    query.bindValue(":capacite", capacite);
    query.bindValue(":id_etab", idEtab);
    query.bindValue(":id_employe", idEmploye);

    if (query.exec()) {
        qDebug() << "Colis added successfully with ID:" << idColis;
        return true;
    } else {
        qDebug() << "Failed to add Colis:" << query.lastError().text();
        qDebug() << "Query:" << query.lastQuery() << "Bound values:" << query.boundValues();
        return false;
    }
}

bool Colis::modifier()
{
    QSqlQuery query(QSqlDatabase::database("EduFlowConnection"));
    query.prepare("UPDATE COLIS SET STATUT = :statut, DATE_SORTIE = :date_sortie, DATE_ARRIVEE_ESTIMEE = :date_arrivee_estimee, "
                  "CAPACITE = :capacite, ID_ETAB = :id_etab, ID_EMPLOYE = :id_employe "
                  "WHERE ID_COLIS = :id_colis");
    query.bindValue(":id_colis", idColis);
    query.bindValue(":statut", statut);
    query.bindValue(":date_sortie", dateSortie); // No need for isValid() check; validation is upstream
    query.bindValue(":date_arrivee_estimee", dateArrivee.isValid() ? QVariant(dateArrivee) : QVariant()); // NULL if invalid
    query.bindValue(":capacite", capacite);
    query.bindValue(":id_etab", idEtab);
    query.bindValue(":id_employe", idEmploye);

    if (query.exec()) {
        qDebug() << "Colis modified successfully with ID:" << idColis;
        return true;
    } else {
        qDebug() << "Failed to modify Colis:" << query.lastError().text();
        return false;
    }
}

bool Colis::supprimer(int idColis)
{
    QSqlQuery query(QSqlDatabase::database("EduFlowConnection"));
    query.prepare("DELETE FROM COLIS WHERE ID_COLIS = :id_colis");
    query.bindValue(":id_colis", idColis);

    if (query.exec()) {
        qDebug() << "Colis deleted successfully with ID:" << idColis;
        return true;
    } else {
        qDebug() << "Failed to delete Colis:" << query.lastError().text();
        return false;
    }
}
