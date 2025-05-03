#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QPdfWriter>
#include <QPainter>
#include <QFileDialog>
#include "qrcodegen.hpp"
#include <QImage>
#include <QLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <cmath>
#include <QMutex>

using qrcodegen::QrCode;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), selectedIdColis(-1), currentSortColumn("ID_COLIS") {
    ui->setupUi(this);
    int ret = ar.connect_arduino();
    switch (ret) {
    case 0:
        qDebug() << "Arduino is available and connected to:" << ar.getarduino_port_name();
        // Connect readyRead only if serial port is valid
        if (ar.getserial()) {
            connect(ar.getserial(), &QSerialPort::readyRead, this, &MainWindow::read_from_arduino);
        }
        break;
    case 1:
        qDebug() << "Arduino is available but not connected to:" << ar.getarduino_port_name();
        break;
    case -1:
        qDebug() << "Arduino connection error";
        break;
    }

    if (!conn.createconnect()) {
        QMessageBox::critical(this, "Erreur", "Impossible de se connecter à la base de données.");
    } else {
        qDebug() << "✅ Database connection established in constructor";
    }

    // Initialize and start QTimer for print_to_lcd
    lcdTimer = new QTimer(this);
    connect(lcdTimer, &QTimer::timeout, this, &MainWindow::print_to_lcd);
    lcdTimer->start(5000); // Call print_to_lcd every 5000 ms (5 seconds)

    // Populate both combo boxes for statut
    QStringList statuts = {"En attente", "En cours", "Livré", "Annulé"};
    ui->comboBox_statut->addItems(statuts);
    ui->comboBox_statut_2->addItems(statuts);

    // Update comboBox_tris with sorting options
    ui->comboBox_tris->clear();
    ui->comboBox_tris->addItems({"ID Colis", "ID Employé", "ID Étab", "Capacité", "Date Arrivée", "Date Sortie", "Statut"});

    // Set initial values for QDateEdit fields
    QDate currentDate = QDate::currentDate();
    ui->lineEdit_dateArrivee->setDate(currentDate);
    ui->lineEdit_dateSortie->setDate(currentDate);
    ui->lineEdit_dateArrivee_2->setDate(currentDate);
    ui->lineEdit_dateSortie_2->setDate(currentDate);

    // Set minimum date to today
    ui->lineEdit_dateArrivee->setMinimumDate(currentDate);
    ui->lineEdit_dateSortie->setMinimumDate(currentDate);
    ui->lineEdit_dateArrivee_2->setMinimumDate(currentDate);
    ui->lineEdit_dateSortie_2->setMinimumDate(currentDate);

    // Connections
    qDebug() << "Connecting Ajouter navigation:" << connect(ui->ajouterColis, &QPushButton::clicked, this, &MainWindow::on_ajouterColis_clicked);
    qDebug() << "Connecting Ajouter button:" << connect(ui->pushButton_ajouter, &QPushButton::clicked, this, &MainWindow::on_pushButton_ajouter_clicked);
    qDebug() << "Connecting Afficher button:" << connect(ui->afficherColis, &QPushButton::clicked, this, &MainWindow::on_afficherColis_clicked);
    qDebug() << "Connecting Table single-click:" << connect(ui->tableWidget_5, &QTableWidget::itemClicked, this, &MainWindow::on_tableWidget_5_clicked);
    qDebug() << "Connecting Supprimer button:" << connect(ui->supprimerColis, &QPushButton::clicked, this, &MainWindow::on_supprimerColis_clicked);
    qDebug() << "Connecting Modifier button:" << connect(ui->modiferColis_2, &QPushButton::clicked, this, &MainWindow::on_modiferColis_2_clicked);
    qDebug() << "Connecting Search field:" << connect(ui->champRecherche_5, &QLineEdit::textChanged, this, &MainWindow::on_champRecherche_5_textChanged);
    qDebug() << "Connecting PDF button:" << connect(ui->pdfEmp_4, &QPushButton::clicked, this, &MainWindow::on_pdfEmp_4_clicked);
    qDebug() << "Connecting Sort combo box:" << connect(ui->comboBox_tris, &QComboBox::currentTextChanged, this, &MainWindow::on_comboBox_tris_currentTextChanged);
    qDebug() << "Connecting QR Code button:" << connect(ui->recEmp_4, &QPushButton::clicked, this, &MainWindow::on_recEmp_4_clicked);
    qDebug() << "Connecting Stats button:" << connect(ui->affichestat, &QPushButton::clicked, this, &MainWindow::on_affichestat_clicked);

    ui->tableWidget_5->setColumnCount(8);
    ui->tableWidget_5->setHorizontalHeaderLabels({"ID Employé", "ID Étab", "Capacité", "Date Arrivée", "Date Sortie", "Statut", "ID Colis", "Modifier"});
    ui->tableWidget_5->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

MainWindow::~MainWindow() {
    if (lcdTimer) {
        lcdTimer->stop();
        delete lcdTimer;
    }
    conn.closeConnection();
    ar.close_arduino();
    delete ui;
}

void MainWindow::clearInputFields() {
    ui->lineEdit_idEmploye->clear();
    ui->lineEdit_idEtab->clear();
    ui->capacite->clear();
    QDate currentDate = QDate::currentDate();
    ui->lineEdit_dateArrivee->setDate(currentDate);
    ui->lineEdit_dateSortie->setDate(currentDate);
    ui->comboBox_statut->setCurrentIndex(0);
    ui->lineEdit_idEmploye_2->clear();
    ui->lineEdit_idEtab_2->clear();
    ui->capacite_2->clear();
    ui->lineEdit_dateArrivee_2->setDate(currentDate);
    ui->lineEdit_dateSortie_2->setDate(currentDate);
    ui->comboBox_statut_2->setCurrentIndex(0);
    selectedIdColis = -1;
    ui->pushButton_ajouter->setText("Ajouter Colis");
    ui->tableWidget_5->clearSelection();
}

void MainWindow::populateTable() {
    ui->tableWidget_5->setRowCount(0);

    QSqlQuery query;
    QString queryStr = "SELECT ID_EMPLOYE, ID_ETAB, CAPACITE, DATE_ARRIVEE_ESTIMEE, DATE_SORTIE, STATUT, ID_COLIS "
                       "FROM DEEPSIGHT.COLIS";
    if (!ui->champRecherche_5->text().isEmpty()) {
        queryStr += " WHERE TO_CHAR(ID_COLIS) LIKE :id_colis";
    }
    queryStr += " ORDER BY " + currentSortColumn + " ASC";

    query.prepare(queryStr);
    if (!ui->champRecherche_5->text().isEmpty()) {
        query.bindValue(":id_colis", "%" + ui->champRecherche_5->text() + "%");
    }

    if (query.exec()) {
        int row = 0;
        while (query.next()) {
            ui->tableWidget_5->insertRow(row);
            ui->tableWidget_5->setItem(row, 0, new QTableWidgetItem(query.value("ID_EMPLOYE").toString()));
            ui->tableWidget_5->setItem(row, 1, new QTableWidgetItem(query.value("ID_ETAB").toString()));
            ui->tableWidget_5->setItem(row, 2, new QTableWidgetItem(query.value("CAPACITE").toString()));
            ui->tableWidget_5->setItem(row, 3, new QTableWidgetItem(query.value("DATE_ARRIVEE_ESTIMEE").toString()));
            ui->tableWidget_5->setItem(row, 4, new QTableWidgetItem(query.value("DATE_SORTIE").toString()));
            ui->tableWidget_5->setItem(row, 5, new QTableWidgetItem(query.value("STATUT").toString()));
            ui->tableWidget_5->setItem(row, 6, new QTableWidgetItem(query.value("ID_COLIS").toString()));

            QPushButton *modifyButton = new QPushButton("Modifier");
            modifyButton->setStyleSheet("background: #0E3B52; color: skyblue; border-radius: 5px; padding: 5px;");
            ui->tableWidget_5->setCellWidget(row, 7, modifyButton);
            connect(modifyButton, &QPushButton::clicked, this, [this, row]() { on_modifyButtonClicked(row); });

            row++;
        }
        qDebug() << "✅ Table populated with" << row << "rows";
    } else {
        qDebug() << "❌ Failed to populate table:" << query.lastError().text();
        QMessageBox::critical(this, "Erreur", "Échec du chargement des données : " + query.lastError().text());
    }
}

void MainWindow::on_ajouterColis_clicked() {
    qDebug() << "Navigating to Ajouter tab";
    ui->distributionsNavBar->setCurrentWidget(ui->ajouterCollis);
    clearInputFields();
}

void MainWindow::on_pushButton_ajouter_clicked() {
    qDebug() << "Ajouter button clicked";
    if (!conn.isOpen() && !conn.createconnect()) {
        qDebug() << "❌ Database connection failed";
        QMessageBox::critical(this, "Erreur", "Base de données non accessible.");
        return;
    }

    QString idEmployeText = ui->lineEdit_idEmploye->text();
    QString idEtabText = ui->lineEdit_idEtab->text();
    QString capaciteText = ui->capacite->text();
    QDate dateArrivee = ui->lineEdit_dateArrivee->date();
    QDate dateSortie = ui->lineEdit_dateSortie->date();
    QString statut = ui->comboBox_statut->currentText();

    QDate currentDate = QDate::currentDate();
    if (dateSortie < currentDate) {
        QMessageBox::warning(this, "Erreur", "La date de sortie ne peut pas être antérieure à aujourd'hui.");
        return;
    }

    if (idEmployeText.isEmpty() || idEtabText.isEmpty() || capaciteText.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Les champs ID Employé, ID Étab et Capacité doivent être remplis.");
        return;
    }

    if (dateArrivee.isValid() && dateArrivee > dateSortie) {
        QMessageBox::warning(this, "Erreur", "La date d'arrivée ne peut pas être postérieure à la date de sortie.");
        return;
    }

    bool ok;
    int id_employe = idEmployeText.toInt(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Erreur", "L'ID Employé doit être un nombre entier.");
        return;
    }

    int id_etab = idEtabText.toInt(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Erreur", "L'ID Étab doit être un nombre entier.");
        return;
    }

    int capacite = capaciteText.toInt(&ok);
    if (!ok || capacite <= 0) {
        QMessageBox::warning(this, "Erreur", "La capacité doit être un nombre entier positif.");
        return;
    }

    QString dateArriveeStr = dateArrivee.isValid() ? dateArrivee.toString("yyyy-MM-dd") : "";
    QString dateSortieStr = dateSortie.isValid() ? dateSortie.toString("yyyy-MM-dd") : QDate::currentDate().toString("yyyy-MM-dd");

    Colis c(id_employe, id_etab, capacite, dateArriveeStr, dateSortieStr, statut);

    if (c.ajouter()) {
        // Get the ID_COLIS of the newly added colis
        QSqlQuery query;
        query.prepare("SELECT ID_COLIS FROM DEEPSIGHT.COLIS WHERE ID_EMPLOYE = :id_employe AND ID_ETAB = :id_etab AND CAPACITE = :capacite AND DATE_ARRIVEE_ESTIMEE = :date_arrivee AND DATE_SORTIE = :date_sortie AND STATUT = :statut");
        query.bindValue(":id_employe", id_employe);
        query.bindValue(":id_etab", id_etab);
        query.bindValue(":capacite", capacite);
        query.bindValue(":date_arrivee", dateArriveeStr);
        query.bindValue(":date_sortie", dateSortieStr);
        query.bindValue(":statut", statut);
        int newColisId = -1;
        if (query.exec() && query.next()) {
            newColisId = query.value("ID_COLIS").toInt();
        }

        // Log the action
        appendColisAction("Ajouter", newColisId, idEmployeText, idEtabText, capaciteText, dateArriveeStr, dateSortieStr, statut,
                          QString("Nouveau colis ajouté avec ID %1").arg(newColisId));

        qDebug() << "✅ Colis ajouté";
        QMessageBox::information(this, "Succès", "Colis ajouté avec succès !");
        clearInputFields();
        populateTable();
    } else {
        qDebug() << "❌ Échec de l'ajout:" << QSqlDatabase::database().lastError().text();
        QMessageBox::critical(this, "Erreur", "Échec de l'ajout du colis : " + QSqlDatabase::database().lastError().text());
    }
}

void MainWindow::on_afficherColis_clicked() {
    qDebug() << "Afficher button clicked";
    if (!conn.isOpen() && !conn.createconnect()) {
        qDebug() << "❌ Database connection failed";
        QMessageBox::critical(this, "Erreur", "Base de données non accessible.");
        return;
    }

    ui->distributionsNavBar->setCurrentWidget(ui->afficherColiss);
    populateTable();
}

void MainWindow::on_tableWidget_5_clicked(QTableWidgetItem *item) {
    qDebug() << "Table single-clicked";
    selectedIdColis = ui->tableWidget_5->item(item->row(), 6)->text().toInt();
    QSqlQuery query;
    query.prepare("SELECT * FROM DEEPSIGHT.COLIS WHERE ID_COLIS = :id_colis");
    query.bindValue(":id_colis", selectedIdColis);
    if (query.exec() && query.next()) {
        ui->lineEdit_idEmploye_2->setText(query.value("ID_EMPLOYE").toString());
        ui->lineEdit_idEtab_2->setText(query.value("ID_ETAB").toString());
        ui->capacite_2->setText(query.value("CAPACITE").toString());
        QString dateArrivee = query.value("DATE_ARRIVEE_ESTIMEE").toString();
        if (!dateArrivee.isEmpty()) {
            ui->lineEdit_dateArrivee_2->setDate(QDate::fromString(dateArrivee, "yyyy-MM-dd"));
        } else {
            ui->lineEdit_dateArrivee_2->setDate(QDate::currentDate());
        }
        ui->lineEdit_dateSortie_2->setDate(QDate::fromString(query.value("DATE_SORTIE").toString(), "yyyy-MM-dd"));
        ui->comboBox_statut_2->setCurrentText(query.value("STATUT").toString());
        qDebug() << "✅ Selected colis loaded into Modifier form";
    } else {
        qDebug() << "❌ Failed to load selected colis:" << query.lastError().text();
        QMessageBox::critical(this, "Erreur", "Impossible de charger les données du colis sélectionné.");
        clearInputFields();
    }
}

void MainWindow::on_supprimerColis_clicked() {
    qDebug() << "Supprimer button clicked";
    if (!conn.isOpen() && !conn.createconnect()) {
        qDebug() << "❌ Database connection failed";
        QMessageBox::critical(this, "Erreur", "Base de données non accessible.");
        return;
    }

    if (selectedIdColis == -1) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un colis à supprimer en cliquant sur une ligne.");
        return;
    }

    // Fetch colis details before deletion for logging
    QSqlQuery query;
    query.prepare("SELECT * FROM DEEPSIGHT.COLIS WHERE ID_COLIS = :id_colis");
    query.bindValue(":id_colis", selectedIdColis);
    QString idEmploye, idEtab, capacite, dateArrivee, dateSortie, statut;
    if (query.exec() && query.next()) {
        idEmploye = query.value("ID_EMPLOYE").toString();
        idEtab = query.value("ID_ETAB").toString();
        capacite = query.value("CAPACITE").toString();
        dateArrivee = query.value("DATE_ARRIVEE_ESTIMEE").toString();
        dateSortie = query.value("DATE_SORTIE").toString();
        statut = query.value("STATUT").toString();
    }

    if (QMessageBox::question(this, "Supprimer", "Voulez-vous supprimer ce colis ?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        if (colis.supprimer(selectedIdColis)) {
            // Log the deletion
            appendColisAction("Supprimer", selectedIdColis, idEmploye, idEtab, capacite, dateArrivee, dateSortie, statut,
                              QString("Colis with ID: %1 has been deleted").arg(selectedIdColis));

            qDebug() << "✅ Colis supprimé";
            QMessageBox::information(this, "Succès", "Colis supprimé avec succès !");
            clearInputFields();
            populateTable();
        } else {
            qDebug() << "❌ Échec de la suppression:" << QSqlDatabase::database().lastError().text();
            QMessageBox::critical(this, "Erreur", "Échec de la suppression du colis : " + QSqlDatabase::database().lastError().text());
        }
    }
}

void MainWindow::on_modiferColis_2_clicked() {
    qDebug() << "Modifier button clicked";
    if (!conn.isOpen() && !conn.createconnect()) {
        qDebug() << "❌ Database connection failed";
        QMessageBox::critical(this, "Erreur", "Base de données non accessible.");
        return;
    }

    if (selectedIdColis == -1) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un colis à modifier en cliquant sur une ligne dans Afficher.");
        return;
    }

    QString idEmployeText = ui->lineEdit_idEmploye_2->text();
    QString idEtabText = ui->lineEdit_idEtab_2->text();
    QString capaciteText = ui->capacite_2->text();
    QDate dateArrivee = ui->lineEdit_dateArrivee_2->date();
    QDate dateSortie = ui->lineEdit_dateSortie_2->date();
    QString statut = ui->comboBox_statut_2->currentText();

    QDate currentDate = QDate::currentDate();
    if (dateSortie < currentDate) {
        QMessageBox::warning(this, "Erreur", "La date de sortie ne peut pas être antérieure à aujourd'hui.");
        return;
    }

    if (idEmployeText.isEmpty() || idEtabText.isEmpty() || capaciteText.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Tous les champs doivent être remplis.");
        return;
    }

    if (dateArrivee.isValid() && dateArrivee > dateSortie) {
        QMessageBox::warning(this, "Erreur", "La date d'arrivée ne peut pas être postérieure à la date de sortie.");
        return;
    }

    bool ok;
    int id_employe = idEmployeText.toInt(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Erreur", "L'ID Employé doit être un nombre entier.");
        return;
    }

    int id_etab = idEtabText.toInt(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Erreur", "L'ID Étab doit être un nombre entier.");
        return;
    }

    int capacite = capaciteText.toInt(&ok);
    if (!ok || capacite <= 0) {
        QMessageBox::warning(this, "Erreur", "La capacité doit être un nombre entier positif.");
        return;
    }

    QString dateArriveeStr = dateArrivee.isValid() ? dateArrivee.toString("yyyy-MM-dd") : "";
    QString dateSortieStr = dateSortie.isValid() ? dateSortie.toString("yyyy-MM-dd") : QDate::currentDate().toString("yyyy-MM-dd");

    // Fetch old values for comparison
    QSqlQuery query;
    query.prepare("SELECT * FROM DEEPSIGHT.COLIS WHERE ID_COLIS = :id_colis");
    query.bindValue(":id_colis", selectedIdColis);
    QString oldIdEmploye, oldIdEtab, oldCapacite, oldDateArrivee, oldDateSortie, oldStatut;
    if (query.exec() && query.next()) {
        oldIdEmploye = query.value("ID_EMPLOYE").toString();
        oldIdEtab = query.value("ID_ETAB").toString();
        oldCapacite = query.value("CAPACITE").toString();
        oldDateArrivee = query.value("DATE_ARRIVEE_ESTIMEE").toString();
        oldDateSortie = query.value("DATE_SORTIE").toString();
        oldStatut = query.value("STATUT").toString();
    }

    // Prepare change details
    QStringList changes;
    if (idEmployeText != oldIdEmploye) changes << QString("ID Employé: %1 -> %2").arg(oldIdEmploye, idEmployeText);
    if (idEtabText != oldIdEtab) changes << QString("ID Étab: %1 -> %2").arg(oldIdEtab, idEtabText);
    if (capaciteText != oldCapacite) changes << QString("Capacité: %1 -> %2").arg(oldCapacite, capaciteText);
    if (dateArriveeStr != oldDateArrivee) changes << QString("Date Arrivée: %1 -> %2").arg(oldDateArrivee, dateArriveeStr);
    if (dateSortieStr != oldDateSortie) changes << QString("Date Sortie: %1 -> %2").arg(oldDateSortie, dateSortieStr);
    if (statut != oldStatut) changes << QString("Statut: %1 -> %2").arg(oldStatut, statut);
    QString changeDetails = changes.isEmpty() ? "Aucun changement" : changes.join("; ");

    Colis c(id_employe, id_etab, capacite, dateArriveeStr, dateSortieStr, statut);
    c.setIdColis(selectedIdColis);

    if (c.modifier()) {
        // Log the modification
        appendColisAction("Modifier", selectedIdColis, idEmployeText, idEtabText, capaciteText, dateArriveeStr, dateSortieStr, statut,
                          QString("Colis ID %1 modifié: %2").arg(selectedIdColis).arg(changeDetails));

        qDebug() << "✅ Colis modifié";
        QMessageBox::information(this, "Succès", "Colis modifié avec succès !");
        clearInputFields();
        populateTable();
        ui->distributionsNavBar->setCurrentWidget(ui->afficherColiss);
    } else {
        qDebug() << "❌ Échec de la modification:" << QSqlDatabase::database().lastError().text();
        QMessageBox::critical(this, "Erreur", "Échec de la modification du colis : " + QSqlDatabase::database().lastError().text());
    }
}

void MainWindow::on_champRecherche_5_textChanged(const QString &text) {
    qDebug() << "Search text changed:" << text;
    if (!conn.isOpen() && !conn.createconnect()) {
        qDebug() << "❌ Database connection failed";
        QMessageBox::critical(this, "Erreur", "Base de données non accessible.");
        return;
    }
    populateTable();
}

void MainWindow::on_pdfEmp_4_clicked() {
    qDebug() << "Generate PDF button clicked";
    if (!conn.isOpen() && !conn.createconnect()) {
        qDebug() << "❌ Database connection failed";
        QMessageBox::critical(this, "Erreur", "Base de données non accessible.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Sauvegarder PDF"), "", tr("PDF Files (*.pdf)"));
    if (fileName.isEmpty()) {
        qDebug() << "❌ No file selected for PDF generation";
        return;
    }
    if (!fileName.endsWith(".pdf", Qt::CaseInsensitive)) {
        fileName += ".pdf";
    }

    QPdfWriter pdfWriter(fileName);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setResolution(300);

    QPainter painter(&pdfWriter);
    painter.setRenderHint(QPainter::Antialiasing);

    const int margin = 100;
    int yPos = margin;
    QFont titleFont("Arial", 14, QFont::Bold);
    QFont font("Arial", 10);
    painter.setFont(titleFont);

    painter.drawText(margin, yPos, "Liste des Colis");
    yPos += 100;

    painter.setFont(font);
    QStringList headers = {"ID Employé", "ID Étab", "Capacité", "Date Arrivée", "Date Sortie", "Statut", "ID Colis"};
    int colWidth = (pdfWriter.width() - 2 * margin) / headers.size();
    for (int i = 0; i < headers.size(); ++i) {
        painter.drawText(margin + i * colWidth, yPos, colWidth, 100, Qt::AlignLeft, headers[i]);
    }
    yPos += 50;
    painter.drawLine(margin, yPos, pdfWriter.width() - margin, yPos);
    yPos += 20;

    QSqlQuery query;
    if (selectedIdColis != -1) {
        query.prepare("SELECT * FROM DEEPSIGHT.COLIS WHERE ID_COLIS = :id_colis");
        query.bindValue(":id_colis", selectedIdColis);
    } else {
        query.prepare("SELECT * FROM DEEPSIGHT.COLIS ORDER BY ID_COLIS ASC");
    }

    if (!query.exec()) {
        qDebug() << "❌ Failed to fetch colis data:" << query.lastError().text();
        QMessageBox::critical(this, "Erreur", "Impossible de récupérer les données des colis.");
        painter.end();
        return;
    }

    while (query.next()) {
        QStringList rowData = {
            query.value("ID_EMPLOYE").toString(),
            query.value("ID_ETAB").toString(),
            query.value("CAPACITE").toString(),
            query.value("DATE_ARRIVEE_ESTIMEE").toString(),
            query.value("DATE_SORTIE").toString(),
            query.value("STATUT").toString(),
            query.value("ID_COLIS").toString()
        };
        for (int col = 0; col < rowData.size(); ++col) {
            painter.drawText(margin + col * colWidth, yPos, colWidth, 100, Qt::AlignLeft, rowData[col]);
        }
        yPos += 50;

        if (yPos > pdfWriter.height() - margin - 1000) {
            pdfWriter.newPage();
            yPos = margin;
            painter.setFont(titleFont);
            painter.drawText(margin, yPos, "Liste des Colis (Suite)");
            yPos += 100;
            painter.setFont(font);
            for (int i = 0; i < headers.size(); ++i) {
                painter.drawText(margin + i * colWidth, yPos, colWidth, 100, Qt::AlignLeft, headers[i]);
            }
            yPos += 50;
            painter.drawLine(margin, yPos, pdfWriter.width() - margin, yPos);
            yPos += 20;
        }
    }

    yPos += 500;
    query.exec();
    while (query.next()) {
        if (selectedIdColis != -1 && query.value("ID_COLIS").toInt() != selectedIdColis) {
            continue;
        }

        QString qrData = QString("ID Colis: %1\nID Employé: %2\nID Étab: %3\nCapacité: %4\nDate Arrivée: %5\nDate Sortie: %6\nStatut: %7")
                             .arg(query.value("ID_COLIS").toString())
                             .arg(query.value("ID_EMPLOYE").toString())
                             .arg(query.value("ID_ETAB").toString())
                             .arg(query.value("CAPACITE").toString())
                             .arg(query.value("DATE_ARRIVEE_ESTIMEE").toString())
                             .arg(query.value("DATE_SORTIE").toString())
                             .arg(query.value("STATUT").toString());

        const QrCode qr = QrCode::encodeText(qrData.toUtf8().constData(), QrCode::Ecc::MEDIUM);
        int size = qr.getSize();
        QImage qrImage(size * 10, size * 10, QImage::Format_RGB32);
        qrImage.fill(Qt::white);
        QPainter qrPainter(&qrImage);
        qrPainter.setPen(Qt::NoPen);
        qrPainter.setBrush(Qt::black);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                if (qr.getModule(x, y)) {
                    qrPainter.drawRect(x * 10, y * 10, 10, 10);
                }
            }
        }
        qrPainter.end();

        int qrSize = 700;
        if (yPos + qrSize > pdfWriter.height() - margin) {
            pdfWriter.newPage();
            yPos = margin;
        }

        painter.setFont(titleFont);
        painter.drawText(margin, yPos, QString("QR Code du Colis %1").arg(query.value("ID_COLIS").toString()));
        yPos += 50;

        QRect qrRect(margin, yPos, qrSize, qrSize);
        painter.drawImage(qrRect, qrImage);
        yPos += qrSize + 100;
    }

    painter.end();
    qDebug() << "✅ PDF generated at:" << fileName;
    QMessageBox::information(this, "Succès", "PDF généré avec succès à : " + fileName);
}

void MainWindow::on_comboBox_tris_currentTextChanged(const QString &text) {
    qDebug() << "Sort selection changed:" << text;

    if (text == "ID Colis") currentSortColumn = "ID_COLIS";
    else if (text == "ID Employé") currentSortColumn = "ID_EMPLOYE";
    else if (text == "ID Étab") currentSortColumn = "ID_ETAB";
    else if (text == "Capacité") currentSortColumn = "CAPACITE";
    else if (text == "Date Arrivée") currentSortColumn = "DATE_ARRIVEE_ESTIMEE";
    else if (text == "Date Sortie") currentSortColumn = "DATE_SORTIE";
    else if (text == "Statut") currentSortColumn = "STATUT";

    qDebug() << "Sorting by:" << currentSortColumn;
    populateTable();
}

void MainWindow::on_recEmp_4_clicked() {
    qDebug() << "Generate QR Code button clicked";
    if (!conn.isOpen() && !conn.createconnect()) {
        qDebug() << "❌ Database connection failed";
        QMessageBox::critical(this, "Erreur", "Base de données non accessible.");
        return;
    }

    if (selectedIdColis == -1) {
        QSqlQuery query;
        query.prepare("SELECT * FROM DEEPSIGHT.COLIS ORDER BY ID_COLIS ASC");
        if (!query.exec()) {
            qDebug() << "❌ Failed to fetch colis data:" << query.lastError().text();
            QMessageBox::critical(this, "Erreur", "Impossible de récupérer les données des colis.");
            return;
        }

        QString dirPath = QFileDialog::getExistingDirectory(this, tr("Sélectionner le dossier de sauvegarde"));
        if (dirPath.isEmpty()) {
            qDebug() << "❌ No directory selected for QR code saving";
            return;
        }

        while (query.next()) {
            QString qrData = QString("ID Colis: %1\nID Employé: %2\nID Étab: %3\nCapacité: %4\nDate Arrivée: %5\nDate Sortie: %6\nStatut: %7")
                                 .arg(query.value("ID_COLIS").toString())
                                 .arg(query.value("ID_EMPLOYE").toString())
                                 .arg(query.value("ID_ETAB").toString())
                                 .arg(query.value("CAPACITE").toString())
                                 .arg(query.value("DATE_ARRIVEE_ESTIMEE").toString())
                                 .arg(query.value("DATE_SORTIE").toString())
                                 .arg(query.value("STATUT").toString());

            const QrCode qr = QrCode::encodeText(qrData.toUtf8().constData(), QrCode::Ecc::MEDIUM);
            int size = qr.getSize();
            QImage qrImage(size * 10, size * 10, QImage::Format_RGB32);
            qrImage.fill(Qt::white);

            QPainter painter(&qrImage);
            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::black);
            for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    if (qr.getModule(x, y)) {
                        painter.drawRect(x * 10, y * 10, 10, 10);
                    }
                }
            }
            painter.end();

            QString fileName = QString("%1/colis_%2.png").arg(dirPath).arg(query.value("ID_COLIS").toString());
            if (!qrImage.save(fileName)) {
                qDebug() << "❌ Failed to save QR Code for colis" << query.value("ID_COLIS").toString();
                QMessageBox::critical(this, "Erreur", QString("Échec de la sauvegarde du QR Code pour colis %1.").arg(query.value("ID_COLIS").toString()));
                return;
            }
            qDebug() << "✅ QR Code saved at:" << fileName;
        }

        QMessageBox::information(this, "Succès", "QR Codes pour tous les colis générés et sauvegardés avec succès dans : " + dirPath);
    } else {
        QSqlQuery query;
        query.prepare("SELECT * FROM DEEPSIGHT.COLIS WHERE ID_COLIS = :id_colis");
        query.bindValue(":id_colis", selectedIdColis);
        if (!query.exec() || !query.next()) {
            qDebug() << "❌ Failed to fetch colis data:" << query.lastError().text();
            QMessageBox::critical(this, "Erreur", "Impossible de récupérer les données du colis.");
            return;
        }

        QString qrData = QString("ID Colis: %1\nID Employé: %2\nID Étab: %3\nCapacité: %4\nDate Arrivée: %5\nDate Sortie: %6\nStatut: %7")
                             .arg(query.value("ID_COLIS").toString())
                             .arg(query.value("ID_EMPLOYE").toString())
                             .arg(query.value("ID_ETAB").toString())
                             .arg(query.value("CAPACITE").toString())
                             .arg(query.value("DATE_ARRIVEE_ESTIMEE").toString())
                             .arg(query.value("DATE_SORTIE").toString())
                             .arg(query.value("STATUT").toString());

        const QrCode qr = QrCode::encodeText(qrData.toUtf8().constData(), QrCode::Ecc::MEDIUM);
        int size = qr.getSize();
        QImage qrImage(size * 10, size * 10, QImage::Format_RGB32);
        qrImage.fill(Qt::white);

        QPainter painter(&qrImage);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::black);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                if (qr.getModule(x, y)) {
                    painter.drawRect(x * 10, y * 10, 10, 10);
                }
            }
        }
        painter.end();

        QString fileName = QFileDialog::getSaveFileName(this, tr("Sauvegarder QR Code"), QString("colis_%1.png").arg(selectedIdColis), tr("PNG Files (*.png)"));
        if (fileName.isEmpty()) {
            qDebug() << "❌ No file selected for QR code saving";
            return;
        }

        if (!fileName.endsWith(".png", Qt::CaseInsensitive)) {
            fileName += ".png";
        }

        if (qrImage.save(fileName)) {
            qDebug() << "✅ QR Code saved at:" << fileName;
            QMessageBox::information(this, "Succès", "QR Code généré et sauvegardé avec succès à : " + fileName);
        } else {
            qDebug() << "❌ Failed to save QR Code";
            QMessageBox::critical(this, "Erreur", "Échec de la sauvegarde du QR Code.");
        }
    }
}

void MainWindow::on_modifyButtonClicked(int row) {
    qDebug() << "Modifier button clicked for row:" << row;
    selectedIdColis = ui->tableWidget_5->item(row, 6)->text().toInt();

    QSqlQuery query;
    query.prepare("SELECT * FROM DEEPSIGHT.COLIS WHERE ID_COLIS = :id_colis");
    query.bindValue(":id_colis", selectedIdColis);
    if (query.exec() && query.next()) {
        ui->lineEdit_idEmploye_2->setText(query.value("ID_EMPLOYE").toString());
        ui->lineEdit_idEtab_2->setText(query.value("ID_ETAB").toString());
        ui->capacite_2->setText(query.value("CAPACITE").toString());
        QString dateArrivee = query.value("DATE_ARRIVEE_ESTIMEE").toString();
        if (!dateArrivee.isEmpty()) {
            ui->lineEdit_dateArrivee_2->setDate(QDate::fromString(dateArrivee, "yyyy-MM-dd"));
        } else {
            ui->lineEdit_dateArrivee_2->setDate(QDate::currentDate());
        }
        ui->lineEdit_dateSortie_2->setDate(QDate::fromString(query.value("DATE_SORTIE").toString(), "yyyy-MM-dd"));
        ui->comboBox_statut_2->setCurrentText(query.value("STATUT").toString());

        ui->distributionsNavBar->setCurrentWidget(ui->modifierColis);
        qDebug() << "✅ Navigated to Modifier page with colis ID:" << selectedIdColis;
    } else {
        qDebug() << "❌ Failed to load colis for modification:" << query.lastError().text();
        QMessageBox::critical(this, "Erreur", "Impossible de charger les données du colis.");
    }
}

void MainWindow::on_affichestat_clicked() {
    qDebug() << "Afficher Stats button clicked";
    if (!conn.isOpen() && !conn.createconnect()) {
        qDebug() << "❌ Database connection failed";
        QMessageBox::critical(this, "Erreur", "Base de données non accessible.");
        return;
    }

    ui->distributionsNavBar->setCurrentWidget(ui->statistiquesEmployes_4);
    displayColisStats();
}

void MainWindow::displayColisStats() {
    // Clear existing layout
    QLayout *layout = ui->framestat->layout();
    if (layout) {
        QLayoutItem *item;
        while ((item = layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete layout;
    }

    QVBoxLayout *vLayout = new QVBoxLayout(ui->framestat);
    ui->framestat->setLayout(vLayout);

    // Query colis statuses
    QSqlQuery query;
    query.prepare("SELECT STATUT, COUNT(*) as count FROM DEEPSIGHT.COLIS GROUP BY STATUT");
    if (!query.exec()) {
        qDebug() << "❌ Failed to fetch colis stats:" << query.lastError().text();
        QMessageBox::critical(this, "Erreur", "Impossible de récupérer les statistiques : " + query.lastError().text());
        return;
    }

    int total = 0;
    QMap<QString, int> stats;
    while (query.next()) {
        QString statut = query.value("STATUT").toString();
        int count = query.value("count").toInt();
        stats[statut] = count;
        total += count;
    }

    // Handle empty data
    if (total == 0) {
        QLabel *noDataLabel = new QLabel("Aucun colis trouvé.", ui->framestat);
        noDataLabel->setStyleSheet("color: #333333; font-size: 20px; font-weight: bold; font-family: 'Arial';");
        noDataLabel->setAlignment(Qt::AlignCenter);
        vLayout->addWidget(noDataLabel);
        vLayout->addStretch();
        return;
    }

    // Create title
    QLabel *titleLabel = new QLabel("Répartition des Colis par Statut", ui->framestat);
    titleLabel->setStyleSheet("color: #0E3B52; font-size: 24px; font-weight: bold; font-family: 'Arial';");
    titleLabel->setAlignment(Qt::AlignCenter);
    vLayout->addWidget(titleLabel);

    // Define custom chart widget
    class ChartWidget : public QWidget {
    public:
        ChartWidget(const QMap<QString, int> &stats, int total, bool isPieChart, QWidget *parent = nullptr)
            : QWidget(parent), stats(stats), total(total), isPieChart(isPieChart), selectedStatut("") {
            setMinimumSize(400, 400);
            setStyleSheet("background: #F5F6F5;");
            setMouseTracking(true);
        }

        void selectStatut(const QString &statut) {
            selectedStatut = statut;
            update();
        }

    protected:
        void paintEvent(QPaintEvent *) override {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);

            // Define chart area
            int chartWidth = width() - 60;
            int chartHeight = height() - 100;
            QRect chartRect(40, 50, chartWidth, chartHeight);

            // Background
            painter.fillRect(rect(), QColor("#F5F6F5"));

            // Chart data
            QStringList statuts = {"En attente", "En cours", "Livré", "Annulé"};
            QList<QColor> colors = {QColor("#FFD700"), QColor("#4682B4"), QColor("#32CD32"), QColor("#FF6347")};
            QMap<QString, QRect> barRects;

            if (isPieChart) {
                // Pie chart
                int chartSize = qMin(chartWidth, chartHeight);
                chartRect = QRect((width() - chartSize) / 2, 50, chartSize, chartSize);
                double startAngle = 0.0;

                for (int i = 0; i < statuts.size(); ++i) {
                    const QString &statut = statuts[i];
                    int count = stats.value(statut, 0);
                    if (count == 0) continue;
                    double angle = (count * 360.0 / total);
                    painter.setBrush(colors[i]);
                    painter.setPen(Qt::NoPen);
                    painter.drawPie(chartRect.adjusted(10, 10, -10, -10), startAngle * 16, angle * 16);

                    // Percentage label
                    double midAngle = startAngle + angle / 2.0;
                    double rad = midAngle * M_PI / 180.0;
                    double radius = chartSize * 0.3;
                    QPointF labelPos = chartRect.center() + QPointF(radius * cos(rad), -radius * sin(rad));
                    painter.setFont(QFont("Arial", 10, QFont::Bold));
                    painter.setPen(Qt::black);
                    double percentage = (count * 100.0 / total);
                    painter.drawText(QRectF(labelPos.x() - 50, labelPos.y() - 20, 100, 40), Qt::AlignCenter,
                                     QString("%1%").arg(percentage, 0, 'f', 1));

                    startAngle += angle;
                }
            } else {
                // Bar chart
                int barCount = 0;
                for (const QString &statut : statuts) {
                    if (stats.value(statut, 0) > 0) barCount++;
                }
                if (barCount == 0) return;

                int barWidth = chartWidth / (barCount * 2);
                int maxCount = 0;
                for (const QString &statut : statuts) {
                    maxCount = qMax(maxCount, stats.value(statut, 0));
                }
                if (maxCount == 0) maxCount = 1;

                int x = 40;
                for (int i = 0; i < statuts.size(); ++i) {
                    const QString &statut = statuts[i];
                    int count = stats.value(statut, 0);
                    if (count == 0) continue;
                    int barHeight = (count * (chartHeight - 50)) / maxCount;
                    QRect barRect(x, chartRect.bottom() - barHeight, barWidth, barHeight);
                    painter.setBrush(colors[i]);
                    painter.setPen(Qt::NoPen);
                    painter.drawRect(barRect);

                    // Percentage and count label
                    painter.setFont(QFont("Arial", 10, QFont::Bold));
                    painter.setPen(Qt::black);
                    double percentage = (count * 100.0 / total);
                    QString label = QString("%1\n%2%").arg(count).arg(percentage, 0, 'f', 1);
                    painter.drawText(barRect.adjusted(0, -50, 0, -10), Qt::AlignCenter, label);

                    barRects[statut] = barRect;
                    x += barWidth * 2;
                }

                // Draw axes
                painter.setPen(Qt::black);
                painter.drawLine(chartRect.bottomLeft(), chartRect.bottomRight()); // X-axis
                painter.drawLine(chartRect.bottomLeft(), chartRect.topLeft());     // Y-axis

                // Draw Y-axis scale (ladder)
                painter.setFont(QFont("Arial", 8));
                int tickCount = qMin(maxCount, 10); // Limit to 10 ticks
                if (tickCount < 1) tickCount = 1;
                for (int i = 0; i <= tickCount; ++i) {
                    int count = (maxCount * i) / tickCount;
                    int y = chartRect.bottom() - (count * (chartHeight - 50)) / maxCount;
                    painter.drawLine(35, y, 40, y); // Tick mark
                    painter.drawText(10, y - 5, 25, 20, Qt::AlignRight, QString::number(count));
                }
            }

            // Draw legend
            int legendX = (width() - 200) / 2;
            int legendY = chartRect.bottom() + 20;
            painter.setFont(QFont("Arial", 12));

            // Sort statuts by count, with selectedStatut first if set
            QList<QPair<QString, int>> sortedStats;
            for (const QString &statut : statuts) {
                int count = stats.value(statut, 0);
                sortedStats.append({statut, count});
            }
            std::sort(sortedStats.begin(), sortedStats.end(),
                      [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                          return a.second > b.second;
                      });
            if (!selectedStatut.isEmpty()) {
                auto it = std::find_if(sortedStats.begin(), sortedStats.end(),
                                       [this](const QPair<QString, int> &p) { return p.first == selectedStatut; });
                if (it != sortedStats.end()) {
                    QPair<QString, int> selected = *it;
                    sortedStats.erase(it);
                    sortedStats.prepend(selected);
                }
            }

            for (const auto &pair : sortedStats) {
                const QString &statut = pair.first;
                int idx = statuts.indexOf(statut);
                if (idx == -1) continue;
                painter.setBrush(colors[idx]);
                painter.drawRect(legendX, legendY, 20, 20);
                painter.setPen(Qt::black);
                painter.drawText(legendX + 30, legendY + 15, statut);
                legendY += 30;
            }
        }

        void mousePressEvent(QMouseEvent *event) override {
            QPointF pos = event->pos();

            if (isPieChart) {
                // Pie chart interaction
                int chartSize = qMin(width() - 60, height() - 100);
                QRect chartRect((width() - chartSize) / 2, 50, chartSize, chartSize);
                QPointF center = chartRect.center();
                QPointF delta = pos - center;
                double distance = sqrt(delta.x() * delta.x() + delta.y() * delta.y());
                if (distance > chartSize / 2 || distance < 10) return; // Outside or in inner gap

                double angle = atan2(-delta.y(), delta.x()) * 180.0 / M_PI;
                if (angle < 0) angle += 360.0;

                QStringList statuts = {"En attente", "En cours", "Livré", "Annulé"};
                double startAngle = 0.0;
                for (const QString &statut : statuts) {
                    int count = stats.value(statut, 0);
                    if (count == 0) continue;
                    double sliceAngle = (count * 360.0 / total);
                    if (angle >= startAngle && angle < startAngle + sliceAngle) {
                        selectStatut(statut);
                        break;
                    }
                    startAngle += sliceAngle;
                }
            } else {
                // Bar chart interaction
                QStringList statuts = {"En attente", "En cours", "Livré", "Annulé"};
                int barCount = 0;
                for (const QString &statut : statuts) {
                    if (stats.value(statut, 0) > 0) barCount++;
                }
                if (barCount == 0) return;

                int chartWidth = width() - 60;
                int chartHeight = height() - 100;
                QRect chartRect(40, 50, chartWidth, chartHeight);
                int barWidth = chartWidth / (barCount * 2);
                int maxCount = 0;
                for (const QString &statut : statuts) {
                    maxCount = qMax(maxCount, stats.value(statut, 0));
                }
                if (maxCount == 0) maxCount = 1;

                int x = 40;
                for (const QString &statut : statuts) {
                    int count = stats.value(statut, 0);
                    if (count == 0) continue;
                    int barHeight = (count * (chartHeight - 50)) / maxCount;
                    QRect barRect(x, chartRect.bottom() - barHeight, barWidth, barHeight);
                    if (barRect.contains(pos.toPoint())) {
                        selectStatut(statut);
                        break;
                    }
                    x += barWidth * 2;
                }
            }
        }

    private:
        QMap<QString, int> stats;
        int total;
        bool isPieChart;
        QString selectedStatut;
    };

    // Create chart widget
    ChartWidget *chartWidget = new ChartWidget(stats, total, isPieChart, ui->framestat);
    vLayout->addWidget(chartWidget);
    vLayout->addStretch();
    qDebug() << "✅ Colis statistics displayed with " << (isPieChart ? "pie" : "bar") << " chart";
}

void MainWindow::on_style_clicked() {
    isPieChart = !isPieChart; // Toggle chart type
    displayColisStats();      // Redraw chart
}

void MainWindow::on_sentEMP_4_clicked() {
    qDebug() << "Historique Livraisons button clicked";
    if (!conn.isOpen() && !conn.createconnect()) {
        qDebug() << "❌ Database connection failed";
        QMessageBox::critical(this, "Erreur", "Base de données non accessible.");
        return;
    }

    // Fetch colis
    struct ColisRecord {
        QString idColis, idEmploye, idEtab, capacite, dateArrivee, dateSortie, statut;
    };
    QList<ColisRecord> records;
    QMap<QString, int> statusCounts;
    QSqlQuery query;
    query.prepare("SELECT ID_COLIS, ID_EMPLOYE, ID_ETAB, CAPACITE, DATE_ARRIVEE_ESTIMEE, DATE_SORTIE, STATUT "
                  "FROM DEEPSIGHT.COLIS ORDER BY ID_COLIS ASC");
    if (!query.exec()) {
        qDebug() << "❌ Failed to fetch colis data:" << query.lastError().text();
        QMessageBox::critical(this, "Erreur", "Impossible de récupérer les données des colis: " + query.lastError().text());
        return;
    }
    while (query.next()) {
        ColisRecord r;
        r.idColis = query.value("ID_COLIS").toString();
        r.idEmploye = query.value("ID_EMPLOYE").toString();
        r.idEtab = query.value("ID_ETAB").toString();
        r.capacite = query.value("CAPACITE").toString();
        r.dateArrivee = query.value("DATE_ARRIVEE_ESTIMEE").toString();
        r.dateSortie = query.value("DATE_SORTIE").toString();
        r.statut = query.value("STATUT").toString();
        records.append(r);
        statusCounts[r.statut]++;
    }
    qDebug() << "Colis trouvés:" << records.count();

    // Read action log from file
    QFile logFile("colis_action_log.txt");
    QMap<int, QList<ColisAction>> actionHistory;
    if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&logFile);
        in.setEncoding(QStringConverter::Utf8);
        while (!in.atEnd()) {
            QString line = in.readLine();
            // Parse log entry (format: [timestamp] Action: action, Colis ID: id, ...)
            QRegularExpression re("\\[(.+)\\] Action: (\\w+), Colis ID: (\\d+), ID Employé: (.+), ID Étab: (.+), Capacité: (.+), Date Arrivée: (.+), Date Sortie: (.+), Statut: (.+), Details: (.+)");
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                ColisAction action;
                action.timestamp = match.captured(1);
                action.action = match.captured(2);
                action.colisId = match.captured(3).toInt();
                action.id_employe = match.captured(4);
                action.id_etab = match.captured(5);
                action.capacite = match.captured(6);
                action.date_arrivee = match.captured(7);
                action.date_sortie = match.captured(8);
                action.statut = match.captured(9);
                action.details = match.captured(10);
                actionHistory[action.colisId].append(action);
            }
        }
        logFile.close();
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString fileName = QFileDialog::getSaveFileName(
        this, "Enregistrer Historique", QDir::homePath() + "/DeliveryHistory_" + timestamp + ".txt", "Fichiers Texte (*.txt)");
    if (fileName.isEmpty()) {
        qDebug() << "❌ No file selected for history report";
        return;
    }
    if (!fileName.endsWith(".txt", Qt::CaseInsensitive)) {
        fileName += ".txt";
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "❌ Impossible d'ouvrir fichier:" << file.errorString();
        QMessageBox::critical(this, "Erreur", "Impossible d'ouvrir fichier: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "===== HISTORIQUE DES LIVRAISONS =====\n";
    out << "Généré le: " << QDateTime::currentDateTime().toString("dd MMMM yyyy, hh:mm:ss") << "\n";
    out << "Nombre total de colis: " << records.count() << "\n";
    out << "Nombre total d'actions: " << actionHistory.size() << "\n\n";

    // Status summary
    out << "Résumé des Statuts\n";
    out << QString("==================\n");
    QStringList statuts = {"En attente", "En cours", "Livré", "Annulé"};
    for (const QString &status : statuts) {
        out << QString("%1: %2\n").arg(status, -12).arg(statusCounts.value(status, 0));
    }
    out << "\n";

    // Colis details
    out << "Détails des Colis\n";
    out << QString("=================\n");
    for (int i = 0; i < records.count(); ++i) {
        const auto &r = records[i];
        out << QString("Colis #%1 (ID: %2)\n").arg(i + 1).arg(r.idColis);
        out << QString("  Statut       : %1\n").arg(r.statut);
        out << QString("  Capacité     : %1\n").arg(r.capacite);
        out << QString("  Employé      : %1\n").arg(r.idEmploye);
        out << QString("  Établissement: %1\n").arg(r.idEtab);
        out << QString("  Arrivée      : %1\n").arg(r.dateArrivee.isEmpty() ? "N/A" : r.dateArrivee);
        out << QString("  Sortie       : %1\n").arg(r.dateSortie.isEmpty() ? "N/A" : r.dateSortie);
        out << "  Historique des Actions:\n";
        const QList<ColisAction> &actions = actionHistory.value(r.idColis.toInt());
        if (actions.isEmpty()) {
            out << "    Aucun\n";
        } else {
            for (const auto &action : actions) {
                out << QString("    [%1] %2: %3\n")
                .arg(action.timestamp)
                    .arg(action.action)
                    .arg(action.details);
            }
        }
        out << "\n";
    }

    out << "===== Fin du Rapport =====\n";
    file.close();

    qDebug() << "✅ Rapport généré:" << fileName;
    QMessageBox::information(this, "Succès", "Historique des livraisons enregistré avec succès à : " + fileName);
}

void MainWindow::read_from_arduino() {
    if (ar.getserial() && ar.getserial()->isReadable()) {
        QByteArray data = ar.read_from_arduino();
        qDebug() << "Received from Arduino:" << data;
        // Add logic to process Arduino data if needed
    }
}

void MainWindow::print_to_lcd() {
    QFile logFile("crash_log.txt");
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream log(&logFile);
        log << QDateTime::currentDateTime().toString() << ": Entering print_to_lcd\n";
        try {
            static QMutex mutex;
            QMutexLocker locker(&mutex); // Ensure single execution

            // Check database connection
            if (!conn.isOpen() && !conn.createconnect()) {
                static bool dbWarned = false;
                if (!dbWarned) {
                    log << QDateTime::currentDateTime().toString() << ": ❌ Database connection failed\n";
                    qDebug() << "❌ Database connection failed";
                    QMessageBox::critical(this, tr("Erreur"), tr("Base de données non accessible."));
                    dbWarned = true;
                }
                logFile.close();
                return;
            }

            // Check Arduino connection
            if (!ar.getserial() || !ar.getserial()->isOpen()) {
                static bool arduinoWarned = false;
                if (!arduinoWarned) {
                    log << QDateTime::currentDateTime().toString() << ": ❌ Arduino serial port not open or not initialized\n";
                    qDebug() << "❌ Arduino serial port not open or not initialized";
                    QMessageBox::warning(this, tr("Erreur"), tr("Arduino non connecté. Vérifiez la connexion."));
                    arduinoWarned = true;
                }
                logFile.close();
                return;
            }

            // Reset warning flags if connections are restored
            static bool dbWarned = false;
            static bool arduinoWarned = false;
            dbWarned = false;
            arduinoWarned = false;

            QSqlQuery query;
            query.prepare("SELECT nb_et, capacite, ORA_ROWSCN FROM etablissements WHERE id_etab = :id_etab");
            query.bindValue(":id_etab", 10);
            if (query.exec()) {
                if (query.next()) {
                    int nb_et = query.value("nb_et").toInt();
                    int capacite = query.value("capacite").toInt();
                    qint64 ora_rowscn = query.value("ORA_ROWSCN").toLongLong();
                    QByteArray data;

                    static bool wasFull = false;
                    bool isFull = nb_et >= capacite;

                    if (isFull) {
                        data = "FULL\n";
                        if (!wasFull) { // Only update if state changed
                            QSqlQuery updateQuery;
                            updateQuery.prepare("UPDATE etablissements SET date_full = SYSDATE WHERE id_etab = :id_etab AND ORA_ROWSCN = :ora_rowscn");
                            updateQuery.bindValue(":id_etab", 10);
                            updateQuery.bindValue(":ora_rowscn", ora_rowscn);
                            if (updateQuery.exec()) {
                                log << QDateTime::currentDateTime().toString() << ": ✅ Updated date_full for id_etab = 10\n";
                                qDebug() << "✅ Updated date_full for id_etab = 10";
                            } else {
                                log << QDateTime::currentDateTime().toString() << ": ❌ Failed to update date_full: " << updateQuery.lastError().text() << "\n";
                                qDebug() << "❌ Failed to update date_full (possible version conflict):" << updateQuery.lastError().text();
                            }
                            wasFull = true;
                        }
                        log << QDateTime::currentDateTime().toString() << ": ✅ Sent to Arduino: FULL (nb_et =" << nb_et << ", capacite =" << capacite << ")\n";
                        qDebug() << "✅ Sent to Arduino: FULL (nb_et =" << nb_et << ", capacite =" << capacite << ")";
                    } else {
                        data = (QString::number(nb_et) + "\n").toUtf8();
                        wasFull = false; // Reset flag
                        log << QDateTime::currentDateTime().toString() << ": ✅ Sent to Arduino: " << nb_et << "\n";
                        qDebug() << "✅ Sent to Arduino:" << nb_et;
                    }
                    ar.write_to_arduino(data);
                } else {
                    log << QDateTime::currentDateTime().toString() << ": ❌ No data found for id_etab = 10\n";
                    qDebug() << "❌ No data found for id_etab = 10";
                    QMessageBox::warning(this, tr("Erreur"), tr("Aucune donnée trouvée pour l'établissement sélectionné."));
                }
            } else {
                log << QDateTime::currentDateTime().toString() << ": ❌ Query failed: " << query.lastError().text() << "\n";
                qDebug() << "❌ Query failed:" << query.lastError().text();
                QMessageBox::critical(this, tr("Erreur"), tr("Échec de la requête SQL : ") + query.lastError().text());
            }
            log << QDateTime::currentDateTime().toString() << ": Exiting print_to_lcd\n";
        } catch (const std::exception& e) {
            log << QDateTime::currentDateTime().toString() << ": ❌ Exception in print_to_lcd: " << e.what() << "\n";
            qDebug() << "❌ Exception in print_to_lcd:" << e.what();
        } catch (...) {
            log << QDateTime::currentDateTime().toString() << ": ❌ Unknown exception in print_to_lcd\n";
            qDebug() << "❌ Unknown exception in print_to_lcd";
        }
        logFile.close();
    }
}

void MainWindow::appendColisAction(const QString &action, int colisId, const QString &id_employe, const QString &id_etab,
                                   const QString &capacite, const QString &date_arrivee, const QString &date_sortie,
                                   const QString &statut, const QString &details) {
    ColisAction act;
    act.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    act.action = action;
    act.colisId = colisId;
    act.id_employe = id_employe;
    act.id_etab = id_etab;
    act.capacite = capacite;
    act.date_arrivee = date_arrivee;
    act.date_sortie = date_sortie;
    act.statut = statut;
    if (action == "Supprimer") {
        act.details = QString("Colis with ID: %1 has been deleted").arg(colisId);
    } else {
        act.details = details;
    }

    // Append to in-memory log
    colisActions.append(act);

    // Save to persistent log file
    saveActionToLogFile(act);

    qDebug() << "✅ Action logged: " << action << " for Colis ID:" << colisId;
}

void MainWindow::saveActionToLogFile(const ColisAction &action) {
    QFile logFile("colis_action_log.txt");
    if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
        qDebug() << "❌ Failed to open log file:" << logFile.errorString();
        QMessageBox::warning(this, "Erreur", "Impossible d'ouvrir le fichier de log: " + logFile.errorString());
        return;
    }

    QTextStream out(&logFile);
    out.setEncoding(QStringConverter::Utf8);
    out << QString("[%1] Action: %2, Colis ID: %3, ID Employé: %4, ID Étab: %5, Capacité: %6, Date Arrivée: %7, Date Sortie: %8, Statut: %9, Details: %10\n")
               .arg(action.timestamp)
               .arg(action.action)
               .arg(action.colisId)
               .arg(action.id_employe)
               .arg(action.id_etab)
               .arg(action.capacite)
               .arg(action.date_arrivee.isEmpty() ? "N/A" : action.date_arrivee)
               .arg(action.date_sortie.isEmpty() ? "N/A" : action.date_sortie)
               .arg(action.statut)
               .arg(action.details);

    logFile.close();
    qDebug() << "✅ Action saved to colis_action_log.txt";
}
