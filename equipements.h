#ifndef EQUIPEMENTS_H
#define EQUIPEMENTS_H

#include <string>
#include <vector>
#include <QSqlQuery>

class Equipements {
private:
    int idEq;
    std::string nomEq;
    std::string etatEq;
    std::string typeEq;
    int quantiteEq;
    std::vector<unsigned char> photoEq;
    std::string dateEq;
    std::string marqueEq;

public:
    Equipements();
    Equipements(int idEq, std::string nomEq, std::string etatEq, std::string typeEq, int quantiteEq,
                std::vector<unsigned char> photoEq, std::string dateEq, std::string marqueEq);

    // Getters
    int getIdEq() const { return idEq; }
    std::string getNomEq() const { return nomEq; }
    std::string getEtatEq() const { return etatEq; }
    std::string getTypeEq() const { return typeEq; }
    int getQuantiteEq() const { return quantiteEq; }
    std::vector<unsigned char> getPhotoEq() const { return photoEq; }
    std::string getDateEq() const { return dateEq; }
    std::string getMarqueEq() const { return marqueEq; }

    // Setters
    void setNomEq(const std::string& nom) { nomEq = nom; }
    void setEtatEq(const std::string& etat) { etatEq = etat; }
    void setTypeEq(const std::string& type) { typeEq = type; }
    void setQuantiteEq(int quantite) { quantiteEq = quantite; }
    void setPhotoEq(const std::vector<unsigned char>& photo) { photoEq = photo; }
    void setDateEq(const std::string& date) { dateEq = date; }
    void setMarqueEq(const std::string& marque) { marqueEq = marque; }

    bool ajouterEq();
    static QSqlQuery afficherEq();
    bool modifierEq(int id);
    bool supprimerEq(int id);
};

#endif // EQUIPEMENTS_H
