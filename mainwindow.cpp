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
    int id = 1;
    Etablissement E(id , nom.toStdString(), gouv.toStdString() , longe , lat , cap , mail.toStdString() , tel);
    bool test = E.ajouter();
    if(test)
    {
        QMessageBox::information(nullptr, QObject::tr("Valider"), QObject::tr("Ajout effectué avec succès!!"), QMessageBox::Cancel);
        E.afficher(ui->tabV);
        ui->nomEtabInput->clear();
        ui->govInput->clear();
        ui->long_2->clear();
        ui->lat->clear();
        ui->cap->clear();
        ui->mail->clear();
        ui->tel->clear();
        id++;
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


void MainWindow::on_ajouterEmp_8_clicked()
{
    int id = ui->tabV->model()->data(ui->tabV->model()->index(0, 0)).toInt();

    qDebug() << "ID récupéré:" << id;

    if (id == 0) {
        QMessageBox::warning(this, "Erreur", "Aucun ID valide sélectionné.");
        return;
    }

    QString nom1  = (ui->nom->text());
    QString gouv2 = (ui->gov->text());
    float longe3 = (ui->long_3->text().toFloat());
    float lat4 = (ui->lat_2->text().toFloat());
    int cap5 = (ui->cap_2->text().toInt());
    QString mail5 = (ui->mail_2->text());
    int tel6 = (ui->tel_2->text().toInt());

    Etablissement e(nom1.toStdString(), gouv2.toStdString() , longe3 , lat4 , cap5 , mail5.toStdString() , tel6);

    bool test1 = e.modifier(id);
    if (test1) {
        QMessageBox::information(this, "Succès", "L'établissement a été modifié avec succès.");
        e.afficher(ui->tabV);
    }
    else {
        QMessageBox::warning(this, "Erreur", "Échec de la modification.");
    }
}

// cahrger les donnés de l'etablissment selon id dans le formulaire de modification

void MainWindow::on_charger_clicked()
{
    int id = ui->tabV->model()->data(ui->tabV->model()->index(0, 0)).toInt();
    qDebug() << "ID récupéré:" << id;

    if (id == 0) {
        QMessageBox::warning(this, "Erreur", "Aucun ID valide sélectionné.");
        return;
    }

    QSqlQuery query;
    query.prepare("SELECT NOM, GOUVERNORAT, LONGE, LAT, CAPACITE, MAIL, TEL FROM ETABLISSEMENTS WHERE ID_ETAB = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur SQL:" << query.lastError().text();
        return;
    }

    if (query.next()) {
        ui->nom->setText(query.value(0).toString());
        ui->gov->setText(query.value(1).toString());
        ui->long_3->setText(query.value(2).toString());
        ui->lat_2->setText(query.value(3).toString());
        ui->cap_2->setText(query.value(4).toString());
        ui->mail_2->setText(query.value(5).toString());
        ui->tel_2->setText(query.value(6).toString());

        QMessageBox::information(this, "Information", "L'établissement a été chargé pour modification.");
    }
    else {
        QMessageBox::warning(this, "Erreur", "Aucun établissement trouvé avec cet ID.");
        return;
    }

}

