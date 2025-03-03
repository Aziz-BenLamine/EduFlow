#include "employe.h"
#include <QMessageBox>
#include <QSqlQueryModel>
#include <QSqlQuery>

Employe::Employe(int id_employee, std::string nomEmp, std::string prenomEmp, std::string emailEmp, int telephoneEmp, std::string dateN, std::string roleEmp, std::vector<unsigned char> photo, std::string password)
{
    this->id_employee = id_employee;
    this->nomEmp = nomEmp;
    this->prenomEmp = prenomEmp;
    this->emailEmp = emailEmp;
    this->telephoneEmp = telephoneEmp;
    this->dateN = dateN;
    this->roleEmp = roleEmp;
    this->photo = photo;
    this->password = password;
}

//Getters
int Employe::getId_employee()
{
    return id_employee;
}

std::string Employe::getNomEmp()
{
    return nomEmp;
}

std::string Employe::getPrenomEmp()
{
    return prenomEmp;
}

std::string Employe::getEmailEmp()
{
    return emailEmp;
}

int Employe::getTelephoneEmp()
{
    return telephoneEmp;
}

std::string Employe::getDateN()
{
    return dateN;
}

std::string Employe::getRoleEmp()
{
    return roleEmp;
}

std::vector<unsigned char> Employe::getPhoto()
{
    return photo;
}

//Setters

void Employe::setId_employee(int id_employee)
{
    this->id_employee = id_employee;
}

void Employe::setNomEmp(std::string nomEmp)
{
    this->nomEmp = nomEmp;
}

void Employe::setPrenomEmp(std::string prenomEmp)
{
    this->prenomEmp = prenomEmp;
}

void Employe::setEmailEmp(std::string emailEmp)
{
    this->emailEmp = emailEmp;
}

void Employe::setTelephoneEmp(int telephoneEmp)
{
    this->telephoneEmp = telephoneEmp;
}

void Employe::setDateN(std::string dateN)
{
    this->dateN = dateN;
}

void Employe::setRoleEmp(std::string roleEmp)
{
    this->roleEmp = roleEmp;
}

void Employe::setPhoto(std::vector<unsigned char> photo)
{
    this->photo = photo;
}

Employe::Employe()
{
    this->id_employee = 0;
    this->nomEmp = "";
    this->prenomEmp = "";
    this->emailEmp = "";
    this->telephoneEmp = 0;
    this->dateN = "";
    this->roleEmp = "";
    this->photo = {};
}
//CRUD

bool Employe::ajouter()
{
    QSqlQuery query;
    query.prepare("INSERT INTO employe (id_employe, nomEmp, prenomEmp, email, telephone, dateN, role, photo) "
                  "VALUES (:id_employee, :nomEmp, :prenomEmp, :emailEmp, :telephoneEmp, TO_DATE(:dateN, 'DD/MM/YYYY'), :roleEmp, :photo)");
    query.bindValue(":id_employee", id_employee);
    query.bindValue(":nomEmp", QString::fromStdString(nomEmp));
    query.bindValue(":prenomEmp", QString::fromStdString(prenomEmp));
    query.bindValue(":emailEmp", QString::fromStdString(emailEmp));
    query.bindValue(":telephoneEmp", telephoneEmp);
    query.bindValue(":dateN", QString::fromStdString(dateN));
    query.bindValue(":roleEmp", QString::fromStdString(roleEmp));
    query.bindValue(":photo", QByteArray(reinterpret_cast<const char*>(photo.data()), photo.size()));
    return query.exec();
}

//Afficher Employe dans la table: tableEmploye

QSqlQueryModel* Employe::afficher()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT * FROM employe");
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prenom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Telephone"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Daten de naissance"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Role"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Photo"));
    return model;
}

//Supprimer Employe

bool Employe::supprimer(int id_employee)
{
    QSqlQuery query;
    query.prepare("DELETE FROM employe WHERE id_employe = :id_employee");
    query.bindValue(":id_employee", id_employee);
    return query.exec();
}

//Modifier Employe

bool Employe::modifier()
{
    QSqlQuery query;
    query.prepare("UPDATE employe SET nomEmp = :nomEmp, prenomEmp = :prenomEmp, email = :emailEmp, telephone = :telephoneEmp, dateN = TO_DATE(:dateN, 'DD/MM/YYYY'), role = :roleEmp, photo = :photo WHERE id_employe = :id_employee");
    query.bindValue(":id_employee", id_employee);
    query.bindValue(":nomEmp", QString::fromStdString(nomEmp));
    query.bindValue(":prenomEmp", QString::fromStdString(prenomEmp));
    query.bindValue(":emailEmp", QString::fromStdString(emailEmp));
    query.bindValue(":telephoneEmp", telephoneEmp);
    query.bindValue(":dateN", QString::fromStdString(dateN));
    query.bindValue(":roleEmp", QString::fromStdString(roleEmp));
    query.bindValue(":photo", QByteArray(reinterpret_cast<const char*>(photo.data()), photo.size()));
    return query.exec();
}
