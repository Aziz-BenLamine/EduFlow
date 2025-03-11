#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    selectedColisId(0)
{
    ui->setupUi(this);

    if (!dbConnection.createconnect()) {
        QMessageBox::critical(this, "Erreur", "Échec de la connexion à la base de données : " + dbConnection.getDatabase().lastError().text());
        return;
    } else {
        qDebug() << "Database connection established successfully.";
    }

    ui->tableWidget_5->setColumnCount(7);
    ui->tableWidget_5->setHorizontalHeaderLabels({"ID Employé", "ID Colis", "ID Etab", "Capacité",
                                                  "Date Arrivée", "Date Sortie", "Statut"});
    ui->tableWidget_5->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_5->setSelectionMode(QAbstractItemView::SingleSelection);

    QStringList statuts = {"En cours", "Livré", "Annulé"};
    ui->comboBox_statut->addItems(statuts);
    ui->comboBox_statut_2->addItems(statuts);

    connect(ui->tableWidget_5, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::on_tableWidget_5_itemSelectionChanged);

    populateTable();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_distributionsBTN_clicked()
{
    ui->mainApp->setCurrentIndex(3);
}

void MainWindow::on_ajouterColis_clicked()
{
    ui->distributionsNavBar->setCurrentIndex(0);
}

void MainWindow::on_afficherColis_clicked()
{
    ui->distributionsNavBar->setCurrentIndex(1);
    populateTable();
}

void MainWindow::on_modiferColis_clicked()
{
    ui->distributionsNavBar->setCurrentIndex(2);
}

void MainWindow::on_supprimerColis_clicked()
{
    if (selectedColisId == 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un colis dans le tableau.");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirmation",
                                                              "Voulez-vous vraiment supprimer le colis avec ID " + QString::number(selectedColisId) + " ?",
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        Colis colis;
        if (colis.supprimer(selectedColisId)) {
            QMessageBox::information(this, "Succès", "Colis supprimé avec succès.");
            selectedColisId = 0;
            ui->lineEdit_idEmploye_2->clear();
            ui->lineEdit_idColis_2->clear();
            ui->lineEdit_idEtab_2->clear();
            ui->capacite_2->clear();
            ui->lineEdit_dateArrivee_2->setDate(QDate::currentDate());
            ui->lineEdit_dateSortie_2->setDate(QDate::currentDate());
            ui->comboBox_statut_2->setCurrentIndex(0);
            populateTable();
        } else {
            QMessageBox::warning(this, "Erreur", "Échec de la suppression.");
        }
    }
}

void MainWindow::on_pushButton_ajouter_clicked()
{
    bool ok;
    int idEmploye = ui->lineEdit_idEmploye->text().toInt(&ok);
    if (!ok || idEmploye <= 0) {
        QMessageBox::warning(this, "Erreur", "ID Employé doit être un nombre positif.");
        return;
    }
    int idColis = ui->lineEdit_idColis->text().toInt(&ok);
    if (!ok || idColis <= 0) {
        QMessageBox::warning(this, "Erreur", "ID Colis doit être un nombre positif.");
        return;
    }
    int idEtab = ui->lineEdit_idEtab->text().toInt(&ok);
    if (!ok || idEtab <= 0) {
        QMessageBox::warning(this, "Erreur", "ID Etab doit être un nombre positif.");
        return;
    }
    int capacite = ui->capacite->text().toInt(&ok);
    if (!ok || capacite <= 0) {
        QMessageBox::warning(this, "Erreur", "Capacité doit être un nombre positif.");
        return;
    }
    QDate dateArrivee = ui->lineEdit_dateArrivee->date(); // Maps to DATE_ARRIVEE_ESTIMEE
    QDate dateSortie = ui->lineEdit_dateSortie->date();   // Maps to DATE_SORTIE
    QString statut = ui->comboBox_statut->currentText();

    if (!dateSortie.isValid()) {
        QMessageBox::warning(this, "Erreur", "Veuillez entrer une Date Sortie valide (obligatoire).");
        return;
    }

    Colis colis(idColis, idEmploye, idEtab, capacite, dateArrivee, dateSortie, statut);
    if (colis.ajouter()) {
        QMessageBox::information(this, "Succès", "Colis ajouté avec succès.");
        ui->lineEdit_idEmploye->clear();
        ui->lineEdit_idColis->clear();
        ui->lineEdit_idEtab->clear();
        ui->capacite->clear();
        ui->lineEdit_dateArrivee->setDate(QDate::currentDate());
        ui->lineEdit_dateSortie->setDate(QDate::currentDate());
        ui->comboBox_statut->setCurrentIndex(0);
        populateTable();
    } else {
        QSqlQuery query(QSqlDatabase::database("EduFlowConnection"));
        query.prepare("SELECT ID_COLIS FROM COLIS WHERE ID_COLIS = :id_colis");
        query.bindValue(":id_colis", idColis);
        if (query.exec() && query.next()) {
            QMessageBox::warning(this, "Erreur", "Échec de l'ajout : Un colis avec cet ID existe déjà.");
        } else {
            QMessageBox::warning(this, "Erreur", "Échec de l'ajout : " + query.lastError().text());
        }
        qDebug() << "Add failed for Colis ID:" << idColis;
    }
}

void MainWindow::on_pushButton_Modifer_clicked()
{
    bool ok;
    int idColis = ui->lineEdit_idColis_2->text().toInt(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Erreur", "ID Colis doit être un nombre.");
        return;
    }
    int idEmploye = ui->lineEdit_idEmploye_2->text().toInt(&ok);
    if (!ok && !ui->lineEdit_idEmploye_2->text().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "ID Employé doit être un nombre.");
        return;
    }
    int idEtab = ui->lineEdit_idEtab_2->text().toInt(&ok);
    if (!ok && !ui->lineEdit_idEtab_2->text().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "ID Etab doit être un nombre.");
        return;
    }
    int capacite = ui->capacite_2->text().toInt(&ok);
    if (!ok && !ui->capacite_2->text().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Capacité doit être un nombre positif.");
        return;
    }
    QDate dateArrivee = ui->lineEdit_dateArrivee_2->date();
    QDate dateSortie = ui->lineEdit_dateSortie_2->date();
    QString statut = ui->comboBox_statut_2->currentText();

    if (idColis == 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez entrer l'ID du colis à modifier.");
        return;
    }

    Colis colis;
    colis.setIdColis(idColis);
    colis.setIdEmploye(idEmploye > 0 ? idEmploye : 0);
    colis.setIdEtab(idEtab > 0 ? idEtab : 0);
    colis.setCapacite(capacite > 0 ? capacite : 0);
    colis.setDateArrivee(dateArrivee.isValid() ? dateArrivee : QDate());
    colis.setDateSortie(dateSortie);
    colis.setStatut(statut);

    if (colis.modifier()) {
        QMessageBox::information(this, "Succès", "Colis modifié avec succès.");
        ui->lineEdit_idEmploye_2->clear();
        ui->lineEdit_idColis_2->clear();
        ui->lineEdit_idEtab_2->clear();
        ui->capacite_2->clear();
        ui->comboBox_statut_2->setCurrentIndex(0);
        populateTable();
    } else {
        QMessageBox::warning(this, "Erreur", "Échec de la modification.");
    }
}

void MainWindow::on_tableWidget_5_itemSelectionChanged()
{
    QList<QTableWidgetItem*> selectedItems = ui->tableWidget_5->selectedItems();
    if (!selectedItems.isEmpty()) {
        int row = ui->tableWidget_5->row(selectedItems.first());
        selectedColisId = ui->tableWidget_5->item(row, 1)->text().toInt();

        QSqlQuery query(QSqlDatabase::database("EduFlowConnection"));
        query.prepare("SELECT * FROM COLIS WHERE ID_COLIS = :id_colis");
        query.bindValue(":id_colis", selectedColisId);
        if (query.exec() && query.next()) {
            Colis c;
            c.setIdEmploye(query.value("ID_EMPLOYE").toInt());
            c.setIdColis(query.value("ID_COLIS").toInt());
            c.setIdEtab(query.value("ID_ETAB").toInt());
            c.setCapacite(query.value("CAPACITE").toInt());
            c.setDateArrivee(query.value("DATE_ARRIVEE_ESTIMEE").toDate());
            c.setDateSortie(query.value("DATE_SORTIE").toDate());
            c.setStatut(query.value("STATUT").toString());
            populateModifierFields(c);
        } else {
            QMessageBox::warning(this, "Erreur", "Erreur lors de la récupération des données : " + query.lastError().text());
        }
    } else {
        selectedColisId = 0;
        ui->lineEdit_idEmploye_2->clear();
        ui->lineEdit_idColis_2->clear();
        ui->lineEdit_idEtab_2->clear();
        ui->capacite_2->clear();
        ui->lineEdit_dateArrivee_2->setDate(QDate::currentDate());
        ui->lineEdit_dateSortie_2->setDate(QDate::currentDate());
        ui->comboBox_statut_2->setCurrentIndex(0);
    }
}

void MainWindow::populateTable()
{
    QSqlQuery query(QSqlDatabase::database("EduFlowConnection"));
    query.prepare("SELECT * FROM COLIS");
    if (query.exec()) {
        ui->tableWidget_5->setRowCount(0);
        int row = 0;
        while (query.next()) {
            ui->tableWidget_5->insertRow(row);
            ui->tableWidget_5->setItem(row, 0, new QTableWidgetItem(query.value("ID_EMPLOYE").toString()));
            ui->tableWidget_5->setItem(row, 1, new QTableWidgetItem(query.value("ID_COLIS").toString()));
            ui->tableWidget_5->setItem(row, 2, new QTableWidgetItem(query.value("ID_ETAB").toString()));
            ui->tableWidget_5->setItem(row, 3, new QTableWidgetItem(query.value("CAPACITE").toString()));
            ui->tableWidget_5->setItem(row, 4, new QTableWidgetItem(query.value("DATE_ARRIVEE_ESTIMEE").toDate().toString("dd/MM/yyyy")));
            ui->tableWidget_5->setItem(row, 5, new QTableWidgetItem(query.value("DATE_SORTIE").toDate().toString("dd/MM/yyyy")));
            ui->tableWidget_5->setItem(row, 6, new QTableWidgetItem(query.value("STATUT").toString()));
            row++;
        }
        ui->tableWidget_5->resizeColumnsToContents();
    } else {
        QMessageBox::warning(this, "Erreur", "Échec du chargement des données : " + query.lastError().text());
        qDebug() << "SQL Error:" << query.lastError().text();
    }
}

void MainWindow::populateModifierFields(const Colis &colis)
{
    ui->lineEdit_idEmploye_2->setText(QString::number(colis.getIdEmploye()));
    ui->lineEdit_idColis_2->setText(QString::number(colis.getIdColis()));
    ui->lineEdit_idEtab_2->setText(QString::number(colis.getIdEtab()));
    ui->capacite_2->setText(QString::number(colis.getCapacite()));
    ui->lineEdit_dateArrivee_2->setDate(colis.getDateArrivee().isValid() ? colis.getDateArrivee() : QDate::currentDate());
    ui->lineEdit_dateSortie_2->setDate(colis.getDateSortie());
    int statutIndex = ui->comboBox_statut_2->findText(colis.getStatut());
    ui->comboBox_statut_2->setCurrentIndex(statutIndex != -1 ? statutIndex : 0);
}
