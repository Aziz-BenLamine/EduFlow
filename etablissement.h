#ifndef ETABLISSEMENT_H
#define ETABLISSEMENT_H
#include <string>
#include <QSqlQuery>
#include <QTableWidget>

class Etablissement
{
private:
    int id_etab;
    std::string nom;
    std::string gouvernorat;
    float longe;
    float lat;
    int capacite;
    std::string email;
    int tel;
public:
    // constructeur
    Etablissement();
    Etablissement(std::string nom , std::string gouvernorat , float longe , float lat, int capacite , std::string email , int tel);

    // getters

    int getID();
    std::string getNom();
    std::string getGouv();
    float getLonge();
    float getLat();
    int getCap();
    std::string getEmail();
    int getTel();

    // setters

    void setNom( std::string nom);
    void setGouv( std::string gouvernorat);
    void setLonge(float longe);
    void setLat(float longe);
    void setCap( int capacite);
    void setEmail( std::string email);
    void setTel( int tel);

    // les methodes CRUD

    bool ajouter();
    void affichier(QTableWidget * table);
    bool supprimerTous();
    bool modifier(int id_etab);




};

#endif // ETABLISSEMENT_H
