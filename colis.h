#ifndef COLIS_H
#define COLIS_H
#include <QDate>
#include <QString>
#include <QSqlQuery>

class Colis
{
public:
    Colis();
    Colis(int idColis, int idEmploye, int idEtab, int capacite, QDate dateArrivee, QDate dateSortie, QString statut);

    // Getters
    int getIdColis() const { return idColis; }
    int getIdEmploye() const { return idEmploye; }
    int getIdEtab() const { return idEtab; }
    int getCapacite() const { return capacite; }
    QDate getDateArrivee() const { return dateArrivee; }
    QDate getDateSortie() const { return dateSortie; }
    QString getStatut() const { return statut; }

    // Setters
    void setIdColis(int id) { idColis = id; }
    void setIdEmploye(int id) { idEmploye = id; }
    void setIdEtab(int id) { idEtab = id; }
    void setCapacite(int cap) { capacite = cap; }
    void setDateArrivee(QDate date) { dateArrivee = date; }
    void setDateSortie(QDate date) { dateSortie = date; }
    void setStatut(QString stat) { statut = stat; }

    // CRUD Operations
    bool ajouter();
    bool modifier();
    bool supprimer(int idColis);

private:
    int idColis, idEmploye, idEtab, capacite;
    QDate dateArrivee, dateSortie;
    QString statut;
};

#endif // COLIS_H
