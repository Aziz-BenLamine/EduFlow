#ifndef ETABLISSEMENT_H
#define ETABLISSEMENT_H
#include <string>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QTableView>
#include <QMap>

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
    QSqlQueryModel *model;

public:

    // constructeur
    Etablissement(std::string nom , std::string gouvernorat , float longe , float lat, int capacite , std::string email , int tel);
    Etablissement()
    {
        this->id_etab=0;
        this->nom="";
        this->gouvernorat="";
        this->longe=0.0f;
        this->lat=0.0f;
        this->capacite=0;
        this->email="";
        this->tel=0;
    }
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
    void afficher(QTableView *tableView);
    bool supprimer(int id);
    bool supprimerTous();
    bool modifier(int id);

    //New method to get stats
    QMap<QString, int> getStatsByGovernorate();
};

#endif // ETABLISSEMENT_H
