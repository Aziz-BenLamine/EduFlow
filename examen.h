#ifndef EXAMEN_H
#define EXAMEN_H

#include <string>
#include <vector>
#include <QSqlQueryModel>

class Examen
{
private:
    int id_exam;                    // NUMBER -> int
    std::string nomExam;           // VARCHAR2(100 BYTE) -> string
    std::string niveauExam;        // VARCHAR2(50 BYTE) -> string
    std::string matiere;          // VARCHAR2(100 BYTE) -> string
    std::string dateExam;         // DATE -> string (you might want to use a proper date type)
    int dureeExam;                // NUMBER -> int
    int nbQuestionsExam;          // NUMBER -> int
    std::vector<unsigned char> plan; // BLOB -> vector of unsigned char

public:
    Examen();
    Examen(int id_exam, std::string nomExam, std::string niveauExam, std::string matiere,
           std::string dateExam, int dureeExam, int nbQuestionsExam, std::vector<unsigned char> plan);

    // Getters
    int getId_exam();
    std::string getNomExam();
    std::string getNiveauExam();
    std::string getMatiere();
    std::string getDateExam();
    int getDureeExam();
    int getNbQuestionsExam();
    std::vector<unsigned char> getPlan();

    // Setters
    void setId_exam(int id_exam);
    void setNomExam(std::string nomExam);
    void setNiveauExam(std::string niveauExam);
    void setMatiere(std::string matiere);
    void setDateExam(std::string dateExam);
    void setDureeExam(int dureeExam);
    void setNbQuestionsExam(int nbQuestionsExam);
    void setPlan(std::vector<unsigned char> plan);

    // CRUD operations
    bool ajouter();
    bool modifier();
    bool supprimer(int id_exam);
    QSqlQueryModel* afficher();
};

#endif // EXAMEN_H
