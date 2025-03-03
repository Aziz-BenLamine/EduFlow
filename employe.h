#ifndef EMPLOYE_H
#define EMPLOYE_H

#include <string>
#include <vector>
#include <QSqlQueryModel>

class Employe
{
private:
    int id_employee;
    std::string nomEmp;
    std::string prenomEmp;
    std::string emailEmp;
    int telephoneEmp;
    std::string dateN;
    std::string roleEmp;
    std::vector<unsigned char> photo;
    std::string password;

public:
    Employe();
    Employe(int id_employee, std::string nomEmp, std::string prenomEmp, std::string emailEmp, int telephoneEmp, std::string dateN, std::string roleEmp, std::vector<unsigned char> photo, std::string password);
    //Getters
    int getId_employee();
    std::string getNomEmp();
    std::string getPrenomEmp();
    std::string getEmailEmp();
    int getTelephoneEmp();
    std::string getDateN();
    std::string getRoleEmp();
    std::vector<unsigned char> getPhoto();
    std::string getPassword();
    //Setters
    void setId_employee(int id_employee);
    void setNomEmp(std::string nomEmp);
    void setPrenomEmp(std::string prenomEmp);
    void setEmailEmp(std::string emailEmp);
    void setTelephoneEmp(int telephoneEmp);
    void setDateN(std::string dateN);
    void setRoleEmp(std::string roleEmp);
    void setPhoto(std::vector<unsigned char> photo);
    void setPassword(std::string password);

    //CRUD
    bool ajouter();
    bool modifier();
    bool supprimer(int id_employee);
    QSqlQueryModel* afficher();
};

#endif // EMPLOYE_H
