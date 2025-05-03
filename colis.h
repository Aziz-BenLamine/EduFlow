#ifndef COLIS_H
#define COLIS_H

#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>

class Colis {
private:
    int id_employe;
    int id_etab;
    int capacite;
    QString date_arrivee_estimee;
    QString date_sortie;
    QString statut;
    int id_colis;

public:
    Colis();
    Colis(int id_emp, int id_etab, int capacite, QString date_arrivee, QString date_sortie, QString statut, int id_colis = 0);

    int getIdEmploye() const;
    int getIdEtab() const;
    int getCapacite() const;
    QString getDateArrivee() const;
    QString getDateSortie() const;
    QString getStatut() const;
    int getIdColis() const;

    void setIdEmploye(int id);
    void setIdEtab(int id);
    void setCapacite(int c);
    void setDateArrivee(QString date);
    void setDateSortie(QString date);
    void setStatut(QString s);
    void setIdColis(int id);

    bool ajouter();
    QSqlQueryModel* afficher();
    bool modifier();
    bool supprimer(int id);
    bool logHistory(const QString &action, const QString &details);
    bool fetchCurrentState(int id_colis, Colis &out);
};

#endif // COLIS_H
