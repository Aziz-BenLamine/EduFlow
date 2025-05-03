#include "colis.h"
#include <QDebug>
#include <QSqlError>

Colis::Colis() : id_employe(0), id_etab(0), capacite(0), id_colis(0), statut("En cours") {}

Colis::Colis(int id_emp, int id_etab, int capacite, QString date_arrivee, QString date_sortie, QString statut, int id_colis)
    : id_employe(id_emp), id_etab(id_etab), capacite(capacite), date_arrivee_estimee(date_arrivee),
    date_sortie(date_sortie), statut(statut), id_colis(id_colis) {
    QStringList validStatuts = {"En attente", "En cours", "Livré", "Annulé"};
    if (!validStatuts.contains(statut)) {
        qDebug() << "Invalid STATUT in constructor:" << statut << ". Defaulting to 'En attente'.";
        this->statut = "En attente";
    }
}

bool Colis::ajouter() {
    if (capacite <= 0) {
        qDebug() << "Ajout échoué: Capacité invalide";
        return false;
    }
    QSqlQuery query;
    query.prepare("INSERT INTO DEEPSIGHT.COLIS (ID_EMPLOYE, ID_ETAB, CAPACITE, DATE_ARRIVEE_ESTIMEE, DATE_SORTIE, STATUT) "
                  "VALUES (:id_employe, :id_etab, :capacite, TO_DATE(:date_arrivee, 'YYYY-MM-DD'), TO_DATE(:date_sortie, 'YYYY-MM-DD'), :statut)");
    query.bindValue(":id_employe", id_employe);
    query.bindValue(":id_etab", id_etab);
    query.bindValue(":capacite", capacite);
    // Handle empty or null date_arrivee_estimee
    if (date_arrivee_estimee.isEmpty()) {
        query.bindValue(":date_arrivee", QVariant(QVariant::String));
    } else {
        query.bindValue(":date_arrivee", date_arrivee_estimee);
    }
    query.bindValue(":date_sortie", date_sortie);
    query.bindValue(":statut", statut);
    if (!query.exec()) {
        qDebug() << "Erreur ajout:" << query.lastError().text();
        return false;
    }
    // Retrieve the auto-generated ID_COLIS
    query.prepare("SELECT COLIS_SEQ.CURRVAL FROM DUAL");
    if (query.exec() && query.next()) {
        id_colis = query.value(0).toInt();
    }
    QString details = QString("Ajouté: Employé=%1, Étab=%2, Capacité=%3, Statut=%4")
                          .arg(id_employe).arg(id_etab).arg(capacite).arg(statut);
    logHistory("Ajout", details);
    qDebug() << "Colis ajouté, ID:" << id_colis;
    return true;
}

bool Colis::fetchCurrentState(int id_colis, Colis &out) {
    QSqlQuery query;
    query.prepare("SELECT ID_EMPLOYE, ID_ETAB, CAPACITE, DATE_ARRIVEE_ESTIMEE, DATE_SORTIE, STATUT "
                  "FROM DEEPSIGHT.COLIS WHERE ID_COLIS = :id_colis");
    query.bindValue(":id_colis", id_colis);
    if (!query.exec() || !query.next()) {
        qDebug() << "Fetch état échoué pour ID" << id_colis << ":" << query.lastError().text();
        return false;
    }
    out.id_employe = query.value("ID_EMPLOYE").toInt();
    out.id_etab = query.value("ID_ETAB").toInt();
    out.capacite = query.value("CAPACITE").toInt();
    out.date_arrivee_estimee = query.value("DATE_ARRIVEE_ESTIMEE").toString();
    out.date_sortie = query.value("DATE_SORTIE").toString();
    out.statut = query.value("STATUT").toString();
    out.id_colis = id_colis;
    qDebug() << "État récupéré pour ID" << id_colis;
    return true;
}

bool Colis::modifier() {
    if (capacite <= 0 || id_colis <= 0) {
        qDebug() << "Modification échouée: Capacité ou ID invalide";
        return false;
    }

    Colis oldState;
    if (!fetchCurrentState(id_colis, oldState)) {
        qDebug() << "Erreur: État actuel introuvable pour ID" << id_colis;
        return false;
    }

    QSqlQuery query;
    query.prepare("UPDATE DEEPSIGHT.COLIS SET ID_EMPLOYE = :id_employe, ID_ETAB = :id_etab, CAPACITE = :capacite, "
                  "DATE_ARRIVEE_ESTIMEE = TO_DATE(:date_arrivee, 'YYYY-MM-DD'), DATE_SORTIE = TO_DATE(:date_sortie, 'YYYY-MM-DD'), STATUT = :statut "
                  "WHERE ID_COLIS = :id_colis");
    query.bindValue(":id_employe", id_employe);
    query.bindValue(":id_etab", id_etab);
    query.bindValue(":capacite", capacite);
    // Handle empty or null date_arrivee_estimee
    if (date_arrivee_estimee.isEmpty()) {
        query.bindValue(":date_arrivee", QVariant(QVariant::String));
    } else {
        query.bindValue(":date_arrivee", date_arrivee_estimee);
    }
    query.bindValue(":date_sortie", date_sortie);
    query.bindValue(":statut", statut);
    query.bindValue(":id_colis", id_colis);
    if (!query.exec()) {
        qDebug() << "Erreur modification ID" << id_colis << ":" << query.lastError().text();
        return false;
    }

    QStringList changes;
    if (oldState.id_employe != id_employe)
        changes << QString("Employé: %1→%2").arg(oldState.id_employe).arg(id_employe);
    if (oldState.id_etab != id_etab)
        changes << QString("Étab: %1→%2").arg(oldState.id_etab).arg(id_etab);
    if (oldState.capacite != capacite)
        changes << QString("Capacité: %1→%2").arg(oldState.capacite).arg(capacite);
    if (oldState.date_arrivee_estimee != date_arrivee_estimee)
        changes << QString("Arrivée: %1→%2").arg(oldState.date_arrivee_estimee, date_arrivee_estimee);
    if (oldState.date_sortie != date_sortie)
        changes << QString("Sortie: %1→%2").arg(oldState.date_sortie, date_sortie);
    if (oldState.statut != statut)
        changes << QString("Statut: %1→%2").arg(oldState.statut, statut);

    QString details = changes.isEmpty() ? "Aucun changement détecté" : changes.join("; ");
    if (logHistory("Modification", details)) {
        qDebug() << "Modification enregistrée pour ID" << id_colis << ":" << details;
    } else {
        qDebug() << "Échec enregistrement historique pour ID" << id_colis;
    }
    return true;
}

bool Colis::supprimer(int id) {
    QSqlQuery query;
    query.prepare("SELECT ID_EMPLOYE, ID_ETAB, CAPACITE, STATUT FROM DEEPSIGHT.COLIS WHERE ID_COLIS = :id_colis");
    query.bindValue(":id_colis", id);
    QString details;
    if (query.exec() && query.next()) {
        details = QString("Supprimé: Employé=%1, Étab=%2, Capacité=%3, Statut=%4")
                      .arg(query.value("ID_EMPLOYE").toInt())
                      .arg(query.value("ID_ETAB").toInt())
                      .arg(query.value("CAPACITE").toInt())
                      .arg(query.value("STATUT").toString());
        id_colis = id; // Set for logHistory
        logHistory("Suppression", details);
        qDebug() << "Suppression enregistrée pour ID" << id;
    }
    query.prepare("DELETE FROM DEEPSIGHT.COLIS WHERE ID_COLIS = :id_colis");
    query.bindValue(":id_colis", id);
    if (!query.exec()) {
        qDebug() << "Erreur suppression ID" << id << ":" << query.lastError().text();
        return false;
    }
    return true;
}

bool Colis::logHistory(const QString &action, const QString &details) {
    // Note: Oracle schema does not include colis_history table
    // If history logging is required, create the table in Oracle or skip this
    qDebug() << "⚠️ Historique non implémenté: colis_history n'existe pas dans Oracle";
    return true; // Return true to avoid breaking existing logic
}

// Getters & Setters
int Colis::getIdEmploye() const { return id_employe; }
int Colis::getIdEtab() const { return id_etab; }
int Colis::getCapacite() const { return capacite; }
QString Colis::getDateArrivee() const { return date_arrivee_estimee; }
QString Colis::getDateSortie() const { return date_sortie; }
QString Colis::getStatut() const { return statut; }
int Colis::getIdColis() const { return id_colis; }
void Colis::setIdEmploye(int id) { id_employe = id; }
void Colis::setIdEtab(int id) { id_etab = id; }
void Colis::setCapacite(int c) { capacite = c; }
void Colis::setDateArrivee(QString date) { date_arrivee_estimee = date; }
void Colis::setDateSortie(QString date) { date_sortie = date; }
void Colis::setStatut(QString s) {
    QStringList validStatuts = {"En attente", "En cours", "Livré", "Annulé"};
    if (validStatuts.contains(s)) {
        statut = s;
    } else {
        qDebug() << "Erreur: Statut invalide:" << s << ". Defaulting to 'En attente'.";
        statut = "En attente";
    }
}
void Colis::setIdColis(int id) { id_colis = id; }

QSqlQueryModel* Colis::afficher() {
    QSqlQueryModel* model = new QSqlQueryModel();
    model->setQuery("SELECT ID_COLIS, ID_EMPLOYE, ID_ETAB, CAPACITE, DATE_ARRIVEE_ESTIMEE, DATE_SORTIE, STATUT "
                    "FROM DEEPSIGHT.COLIS ORDER BY ID_COLIS ASC");
    model->setHeaderData(0, Qt::Horizontal, "ID Colis");
    model->setHeaderData(1, Qt::Horizontal, "ID Employé");
    model->setHeaderData(2, Qt::Horizontal, "ID Étab");
    model->setHeaderData(3, Qt::Horizontal, "Capacité");
    model->setHeaderData(4, Qt::Horizontal, "Date Arrivée");
    model->setHeaderData(5, Qt::Horizontal, "Date Sortie");
    model->setHeaderData(6, Qt::Horizontal, "Statut");
    return model;
}
