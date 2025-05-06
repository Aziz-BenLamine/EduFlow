#include "examen.h"
#include <QMessageBox>
#include <QSqlQueryModel>
#include <QSqlQuery>

Examen::Examen(int id_exam, std::string nomExam, std::string niveauExam, std::string matiere,
               std::string dateExam, int dureeExam, int nbQuestionsExam, std::vector<unsigned char> plan)
{
    this->id_exam = id_exam;
    this->nomExam = nomExam;
    this->niveauExam = niveauExam;
    this->matiere = matiere;
    this->dateExam = dateExam;
    this->dureeExam = dureeExam;
    this->nbQuestionsExam = nbQuestionsExam;
    this->plan = plan;
}

// Default constructor
Examen::Examen()
{
    this->id_exam = 0;
    this->nomExam = "";
    this->niveauExam = "";
    this->matiere = "";
    this->dateExam = "";
    this->dureeExam = 0;
    this->nbQuestionsExam = 0;
    this->plan = {};
}

// Getters
int Examen::getId_exam()
{
    return id_exam;
}

std::string Examen::getNomExam()
{
    return nomExam;
}

std::string Examen::getNiveauExam()
{
    return niveauExam;
}

std::string Examen::getMatiere()
{
    return matiere;
}

std::string Examen::getDateExam()
{
    return dateExam;
}

int Examen::getDureeExam()
{
    return dureeExam;
}

int Examen::getNbQuestionsExam()
{
    return nbQuestionsExam;
}

std::vector<unsigned char> Examen::getPlan()
{
    return plan;
}

// Setters
void Examen::setId_exam(int id_exam)
{
    this->id_exam = id_exam;
}

void Examen::setNomExam(std::string nomExam)
{
    this->nomExam = nomExam;
}

void Examen::setNiveauExam(std::string niveauExam)
{
    this->niveauExam = niveauExam;
}

void Examen::setMatiere(std::string matiere)
{
    this->matiere = matiere;
}

void Examen::setDateExam(std::string dateExam)
{
    this->dateExam = dateExam;
}

void Examen::setDureeExam(int dureeExam)
{
    this->dureeExam = dureeExam;
}

void Examen::setNbQuestionsExam(int nbQuestionsExam)
{
    this->nbQuestionsExam = nbQuestionsExam;
}

void Examen::setPlan(std::vector<unsigned char> plan)
{
    this->plan = plan;
}

// CRUD operations
bool Examen::ajouter()
{
    QSqlQuery query;
    query.prepare("INSERT INTO examens (ID_EXAM, NOMEXAM, NIVEAUEXAM, MATIERE, DATEEXAM, DUREEEXAM, NBQUESTIONSEXAM, PLAN) "
                  "VALUES (:id_exam, :nomExam, :niveauExam, :matiere, TO_DATE(:dateExam, 'DD/MM/YYYY'), :dureeExam, :nbQuestionsExam, :plan)");
    query.bindValue(":id_exam", id_exam);
    query.bindValue(":nomExam", QString::fromStdString(nomExam));
    query.bindValue(":niveauExam", QString::fromStdString(niveauExam));
    query.bindValue(":matiere", QString::fromStdString(matiere));
    query.bindValue(":dateExam", QString::fromStdString(dateExam));
    query.bindValue(":dureeExam", dureeExam);
    query.bindValue(":nbQuestionsExam", nbQuestionsExam);
    query.bindValue(":plan", QByteArray(reinterpret_cast<const char*>(plan.data()), plan.size()));
    return query.exec();
}

QSqlQueryModel* Examen::afficher()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT * FROM examens");
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom Examen"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Niveau"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Matière"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Date"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Durée"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Nb Questions"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Plan"));
    return model;
}

bool Examen::supprimer(int id_exam)
{
    QSqlQuery query;
    query.prepare("DELETE FROM examens WHERE ID_EXAM = :id_exam");
    query.bindValue(":id_exam", id_exam);
    return query.exec();
}

bool Examen::modifier()
{
    QSqlQuery query;
    query.prepare("UPDATE examens SET NOMEXAM = :nomExam, NIVEAUEXAM = :niveauExam, MATIERE = :matiere, "
                  "DATEEXAM = TO_DATE(:dateExam, 'DD/MM/YYYY'), DUREEEXAM = :dureeExam, NBQUESTIONSEXAM = :nbQuestionsExam, "
                  "PLAN = :plan WHERE ID_EXAM = :id_exam");
    query.bindValue(":id_exam", id_exam);
    query.bindValue(":nomExam", QString::fromStdString(nomExam));
    query.bindValue(":niveauExam", QString::fromStdString(niveauExam));
    query.bindValue(":matiere", QString::fromStdString(matiere));
    query.bindValue(":dateExam", QString::fromStdString(dateExam));
    query.bindValue(":dureeExam", dureeExam);
    query.bindValue(":nbQuestionsExam", nbQuestionsExam);
    query.bindValue(":plan", QByteArray(reinterpret_cast<const char*>(plan.data()), plan.size()));
    return query.exec();  // Fixed: changed return true to query.exec()
}
