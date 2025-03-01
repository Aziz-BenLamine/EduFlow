#ifndef EMPLOYE_H
#define EMPLOYE_H

#include <string>
#include <vector>

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

    public:
        Employe();
        Employe(int id_employee, std::string nomEmp, std::string prenomEmp, std::string emailEmp, int telephoneEmp, std::string dateN, std::string roleEmp, std::vector<unsigned char> photo);
        //Getters
        int getId_employee();
        std::string getNomEmp();
        std::string getPrenomEmp();
        std::string getEmailEmp();
        int getTelephoneEmp();
        std::string getDateN();
        std::string getRoleEmp();
        std::vector<unsigned char> getPhoto();
        //Setters
        void setId_employee(int id_employee);
        void setNomEmp(std::string nomEmp);
        void setPrenomEmp(std::string prenomEmp);
        void setEmailEmp(std::string emailEmp);
        void setTelephoneEmp(int telephoneEmp);
        void setDateN(std::string dateN);
        void setRoleEmp(std::string roleEmp);
        void setPhoto(std::vector<unsigned char> photo);

        //CRUD
        bool ajouter();
        bool modifier();
        bool supprimer();
        std::vector<Employe> afficher();
};

#endif // EMPLOYE_H
