#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QMessageBox"
#include "etablissement.h"
#include <string>
#include <QSqlTableModel>
#include <QTableView>
#include <QModelIndex>
#include <QDebug>
#include <QSqlError>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_ajouterEmp_clicked()
{
    ui->employeesNavBar->setCurrentIndex(0);
}


void MainWindow::on_afficherEmp_clicked()
{
    ui->employeesNavBar->setCurrentIndex(1);
}


void MainWindow::on_modiferEmp_clicked()
{
    ui->employeesNavBar->setCurrentIndex(2);
}


void MainWindow::on_statsEmp_clicked()
{
    ui->employeesNavBar->setCurrentIndex(3);
}


void MainWindow::on_etablissementBTN_clicked()
{
    ui->mainApp->setCurrentIndex(1);
}


void MainWindow::on_employesBTN_clicked()
{
    ui->mainApp->setCurrentIndex(0);
    ui->employeesNavBar->setCurrentIndex(0);
}

//Etablissements Navbar

void MainWindow::on_ajouterEtab_clicked()
{
    ui->etablissementsNavBar->setCurrentIndex(0);
}

void MainWindow::on_afficherEtab_clicked()
{
    ui->etablissementsNavBar->setCurrentIndex(1);
}

void MainWindow::on_modiferEtab_clicked()
{
    ui->etablissementsNavBar->setCurrentIndex(2);
}

void MainWindow::on_statsEtab_clicked()
{
    ui->etablissementsNavBar->setCurrentIndex(3);
}


void MainWindow::on_distributionsBTN_clicked()
{
    ui->mainApp->setCurrentIndex(3);
}


void MainWindow::on_equipementsBTN_clicked()
{
    ui->mainApp->setCurrentIndex(4);
}


void MainWindow::on_ajouterColis_clicked()
{
    ui->distributionsNavBar->setCurrentIndex(0);
}


void MainWindow::on_afficherColis_clicked()
{
    ui->distributionsNavBar->setCurrentIndex(1);
}


void MainWindow::on_modiferColis_clicked()
{
    ui->distributionsNavBar->setCurrentIndex(2);
}


void MainWindow::on_statsColis_clicked()
{
    ui->distributionsNavBar->setCurrentIndex(3);
}


void MainWindow::on_ajouterEq_clicked()
{
    ui->equipementsNavBar->setCurrentIndex(0);
}


void MainWindow::on_afficherEq_clicked()
{
    ui->equipementsNavBar->setCurrentIndex(1);
}


void MainWindow::on_modiferEq_clicked()
{
    ui->equipementsNavBar->setCurrentIndex(2);
}


void MainWindow::on_statsEq_clicked()
{
    ui->equipementsNavBar->setCurrentIndex(3);
}


void MainWindow::on_examensBTN_clicked()
{
    ui->mainApp->setCurrentIndex(2);
}


void MainWindow::on_ajouterExam_clicked()
{
    ui->examensNavBar->setCurrentIndex(0);
}

void MainWindow::on_afficherExam_clicked()
{
    ui->examensNavBar->setCurrentIndex(1);
}


void MainWindow::on_modiferExam_clicked()
{
    ui->examensNavBar->setCurrentIndex(2);
}


void MainWindow::on_statsExam_clicked()
{
    ui->examensNavBar->setCurrentIndex(3);
}


void MainWindow::on_pushButton_clicked()
{
    ui->login_app->setCurrentIndex(1);
}


void MainWindow::on_deconnexionBTN_clicked()
{
    ui->login_app->setCurrentIndex(0);
}


void MainWindow::on_ajouterEmp_4_clicked()
{

}

// ajout un etablissement

void MainWindow::on_ajouterEtab_2_clicked()
{
    QString nom = ui->nomEtabInput->text();
    QString gouv = ui->govInput->text();
    float longe = ui->long_2->text().toFloat();
    float lat = ui->lat->text().toFloat();
    int cap = ui->cap->text().toInt();
    QString mail = ui->mail->text();
    int tel = ui->tel->text().toInt();

    QRegularExpression regexNom("^[a-zA-ZÀ-ÖØ-öø-ÿ ]+$");
    QRegularExpression regexTel("^[0-9]+$");
    bool nomValide = regexNom.match(nom).hasMatch();
    bool gouvValide = regexNom.match(gouv).hasMatch();
    bool telValide = regexTel.match(ui->tel->text()).hasMatch();
    bool longeValide = longe > 0;
    bool latValide = lat > 0;
    bool mailValide = mail.contains("@") && mail.contains(".");
    bool capValide = cap > 0;

    if (!nomValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("Le nom doit contenir uniquement des lettres et des espaces!"), QMessageBox::Ok);
        return ;
    } else if (!gouvValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("Le gouvernorat doit contenir uniquement des lettres et des espaces!"), QMessageBox::Ok);
    } else if (!longeValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("La longitude doit être un nombre positif!"), QMessageBox::Ok);
        return ;
    } else if (!latValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("La latitude doit être un nombre positif!"), QMessageBox::Ok);
        return ;
    } else if (!mailValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("L'email doit contenir '@' et '.'!"), QMessageBox::Ok);
        return ;
    }
    else if (!telValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("Le numéro de téléphone doit contenir uniquement des chiffres!"), QMessageBox::Ok);
        return ;
    } else if (!capValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("La capacité doit être un nombre positif!"), QMessageBox::Ok);
        return ;
    }
    Etablissement E(nom.toStdString(), gouv.toStdString() , longe , lat , cap , mail.toStdString() , tel);
    bool test = E.ajouter();
    if(test)
    {
        QMessageBox::information(nullptr, QObject::tr("Valider"), QObject::tr("Ajout effectué avec succès!!"), QMessageBox::Cancel);
        ui->nomEtabInput->clear();
        ui->govInput->clear();
        ui->long_2->clear();
        ui->lat->clear();
        ui->cap->clear();
        ui->mail->clear();
        ui->tel->clear();
    }
    else
    {
        QMessageBox::critical(nullptr, QObject::tr("Erreur"), QObject::tr("Ajout non effectué!!"), QMessageBox::Cancel);
    }

}

// affichier les etablissements

void MainWindow::on_affBtn_clicked()
{
    Etablissement E;
    E.afficher(ui->tabV);
}


// supprimer les etablissements

void MainWindow::on_checkBox_2_stateChanged(int arg1)
{
    if (arg1 == Qt::Checked) {
        Etablissement E;
        if (E.supprimerTous()) {
            E.afficher(ui->tabV);

            QMessageBox::information(this, "Suppression réussie", "Tous les établissements ont été supprimés.");
        }
        else {
            QMessageBox::warning(this, "Erreur", "Échec de la suppression des établissements.");
        }
    }
    ui->checkBox_2->setChecked(false);
}

void MainWindow::on_checkBox_stateChanged(int arg1)
{
    if (arg1 == Qt::Checked) {
        QModelIndex index = ui->tabV->currentIndex();
        if (!index.isValid()) {
            QMessageBox::warning(this, "Sélection invalide", "Veuillez sélectionner un établissement à supprimer.");
            return;
        }

        int id = ui->tabV->model()->data(ui->tabV->model()->index(index.row(), 0)).toInt();

        Etablissement E;
        bool test = E.supprimer(id);
        if (test) {
            E.afficher(ui->tabV);
            QMessageBox::information(this, "Suppression réussie", "L'établissement a été supprimé avec succès.");
        } else {
            QMessageBox::warning(this, "Erreur", "Échec de la suppression de l'établissement. Vérifiez si l'ID existe.");
        }
    }
    ui->checkBox->setChecked(false);
}



