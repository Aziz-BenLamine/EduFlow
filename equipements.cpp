#include "equipements.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <map>

Equipements::Equipements()
    : idEq(0), nomEq(""), etatEq(""), typeEq(""), quantiteEq(0), photoEq(), dateEq(""), marqueEq("") {}

Equipements::Equipements(int idEq, std::string nomEq, std::string etatEq, std::string typeEq, int quantiteEq,
                         std::vector<unsigned char> photoEq, std::string dateEq, std::string marqueEq)
    : idEq(idEq), nomEq(nomEq), etatEq(etatEq), typeEq(typeEq), quantiteEq(quantiteEq),
    photoEq(photoEq), dateEq(dateEq), marqueEq(marqueEq) {}

bool Equipements::ajouterEq() {
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM EQUIPEMENTS WHERE ID_EQ = :IdEq");
    query.bindValue(":IdEq", idEq);
    if (!query.exec() || !query.next()) {
        qDebug() << "Erreur lors de la vérification de l'ID :" << query.lastError().text();
        return false;
    }
    if (query.value(0).toInt() > 0) {
        qDebug() << "Erreur : L'ID " << idEq << " existe déjà.";
        return false;
    }

    if (quantiteEq <= 0) {
        qDebug() << "Erreur : Quantité invalide (" << quantiteEq << ").";
        return false;
    }

    QByteArray imageData;
    if (!photoEq.empty()) {
        imageData = QByteArray(reinterpret_cast<const char*>(photoEq.data()), photoEq.size());
    }

    query.prepare("INSERT INTO EQUIPEMENTS (ID_EQ, NOM_EQ, TYPEEQ, ETATEQ, MARQUEEQ, QT, DATEEQ, IMAGE_EQ) "
                  "VALUES (:IdEq, :nomEq, :typeEq, :etatEq, :marqueEq, :quantiteEq, :dateEq, :photoEq)");

    query.bindValue(":IdEq", idEq);
    query.bindValue(":nomEq", QString::fromStdString(nomEq));
    query.bindValue(":typeEq", QString::fromStdString(typeEq));
    query.bindValue(":etatEq", QString::fromStdString(etatEq));
    query.bindValue(":marqueEq", QString::fromStdString(marqueEq));
    query.bindValue(":quantiteEq", quantiteEq);
    query.bindValue(":dateEq", QString::fromStdString(dateEq)); // SQLite utilise YYYY-MM-DD directement
    query.bindValue(":photoEq", imageData);

    if (!query.exec()) {
        qDebug() << "Erreur SQL lors de l'ajout :" << query.lastError().text();
        return false;
    }
    return true;
}

QSqlQuery Equipements::afficherEq() {
    QSqlQuery query;
    query.prepare("SELECT ID_EQ, NOM_EQ, TYPEEQ, ETATEQ, MARQUEEQ, QT, DATEEQ, IMAGE_EQ "
                  "FROM EQUIPEMENTS ORDER BY ID_EQ");
    if (!query.exec()) {
        qDebug() << "Erreur lors de l'affichage :" << query.lastError().text();
    }
    return query;
}

bool Equipements::modifierEq(int id) {
    if (quantiteEq <= 0) {
        qDebug() << "Erreur : Quantité invalide (" << quantiteEq << ").";
        return false;
    }

    QSqlQuery query;
    QByteArray imageData;
    if (!photoEq.empty()) {
        imageData = QByteArray(reinterpret_cast<const char*>(photoEq.data()), photoEq.size());
    }

    query.prepare("UPDATE EQUIPEMENTS SET "
                  "NOM_EQ = :nomEq, "
                  "TYPEEQ = :typeEq, "
                  "ETATEQ = :etatEq, "
                  "MARQUEEQ = :marqueEq, "
                  "QT = :quantiteEq, "
                  "DATEEQ = :dateEq, "
                  "IMAGE_EQ = :photoEq "
                  "WHERE ID_EQ = :id");

    query.bindValue(":nomEq", QString::fromStdString(nomEq));
    query.bindValue(":typeEq", QString::fromStdString(typeEq));
    query.bindValue(":etatEq", QString::fromStdString(etatEq));
    query.bindValue(":marqueEq", QString::fromStdString(marqueEq));
    query.bindValue(":quantiteEq", quantiteEq);
    query.bindValue(":dateEq", QString::fromStdString(dateEq));
    query.bindValue(":photoEq", imageData);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur SQL lors de la modification :" << query.lastError().text();
        return false;
    }
    return true;
}

bool Equipements::supprimerEq(int id) {
    QSqlQuery query;
    query.prepare("DELETE FROM EQUIPEMENTS WHERE ID_EQ = :IdEq");
    query.bindValue(":IdEq", id);
    if (!query.exec()) {
        qDebug() << "Erreur SQL lors de la suppression :" << query.lastError().text();
        return false;
    }
    return true;
}

// equipements.cpp
#include "equipements.h"
#include <map>

// Constructor and other methods...
/*
std::map<std::string, int> Equipements::getQuantiteStatistics() {
    std::map<std::string, int> stats;
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open("equipements.db", &db) != SQLITE_OK) {
        return stats; // Return empty map on error
    }

    const char *sql = "SELECT quantite FROM equipements;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return stats;
    }

    // Define quantity ranges
    std::map<std::string, int> ranges = {
        {"1-5", 0},
        {"6-10", 0},
        {"11-20", 0},
        {"21+", 0}
    };

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int quantite = sqlite3_column_int(stmt, 0);
        if (quantite >= 1 && quantite <= 5) {
            ranges["1-5"]++;
        } else if (quantite >= 6 && quantite <= 10) {
            ranges["6-10"]++;
        } else if (quantite >= 11 && quantite <= 20) {
            ranges["11-20"]++;
        } else if (quantite >= 21) {
            ranges["21+"]++;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // Only include non-zero ranges in the final stats
    for (const auto &pair : ranges) {
        if (pair.second > 0) {
            stats[pair.first] = pair.second;
        }
    }

    return stats;
}
*/
