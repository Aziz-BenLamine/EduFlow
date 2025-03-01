#include "employe.h"
#include <QMessageBox>
#include <QSqlQueryModel>
#include <QSqlQuery>

Employe::Employe(int id_employee, std::string nomEmp, std::string prenomEmp, std::string emailEmp, int telephoneEmp, std::string dateN, std::string roleEmp, std::vector<unsigned char> photo)
{
    this->id_employee = id_employee;
    this->nomEmp = nomEmp;
    this->prenomEmp = prenomEmp;
    this->emailEmp = emailEmp;
    this->telephoneEmp = telephoneEmp;
    this->dateN = dateN;
    this->roleEmp = roleEmp;
    this->photo = photo;
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
//Ajouter Employe dans la base de donnes

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
