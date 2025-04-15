#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QRegularExpression>
#include <QDate>
#include <QPainter>
#include <QPdfWriter>
#include <QVBoxLayout>
#include "statswidgetemp.h"
#include <QDateTime>
//GG

#include "employe.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , newPhotoSelected(false)
    , faceRecognitionActive(false)
    , consecutiveDetections(0)
    , emotionRecognitionActive(false)
    , neutralFrameCount(0)
    , lastSentiment("None")
    , cheerUpQuote("")
    ,happyFrameCount(0)
{
    ui->setupUi(this);
    ui->cameraLabel->hide();
    refreshEmployeeTable();
    connect(ui->tableEmploye, &QTableView::clicked, this, &MainWindow::onEmployeeTableClicked);
    refreshStats();

    qDebug() << "Qt Version:" << QT_VERSION_STR;

    // Initialize facial recognition
    cap = cv::VideoCapture(); // Camera not opened yet
    faceCascade.load("C:/opencv_contrib-4.9.0/install/etc/haarcascades/haarcascade_frontalface_default.xml");
    if (faceCascade.empty()) {
        qDebug() << "Error: Could not load cascade file.";
    }

    recognizer = cv::face::LBPHFaceRecognizer::create();
    recognizer->read("face_model.yml");
    if (recognizer->empty()) {
        qDebug() << "Error: Could not load face model.";
    }

    user_names = {"Aziz"}; // Match your trained model
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateFrame);
    // Initialize smile detection
    if (!smileCascade.load("C:/opencv_contrib-4.9.0/install/etc/haarcascades/haarcascade_smile.xml")) {
        qDebug() << "Error: Could not load smile cascade.";
    }

    // Initialize cheer-up messages
    cheerMessages << "Keep Pushing Forward!"
                  << "You're Making Progress!"
                  << "Stay Focused, Stay Strong!"
                  << "One Step at a Time!"
                  << "You've Got This!"
                  << "Turn Challenges into Wins!"
                  << "Your Effort Counts!"
                  << "Keep Up the Momentum!";
    // Style labels
    ui->cameraLabel->setScaledContents(true);
    ui->emotionLabel->setScaledContents(true);

    // Initialize toggle timer
    toggleTimer = new QTimer(this);
    connect(toggleTimer, &QTimer::timeout, this, &MainWindow::toggleEmotionRecognition);
    toggleTimer->start(60000); // Start toggling every 60 seconds
}

MainWindow::~MainWindow()
{
    if (cap.isOpened()) {
        cap.release();
    }
    delete timer;
    delete toggleTimer;
    delete ui;
}

//FACIAL RECOGNITION





//STATS
void MainWindow::refreshStats() {
    StatsWidgetEmp *statsWidget = qobject_cast<StatsWidgetEmp*>(ui->statsWidget);
    if (statsWidget) {
        statsWidget->updateStats();
        qDebug() << "Refreshed stats on ui->statsWidget:" << statsWidget;
    } else {
        qDebug() << "Error: ui->statsWidget is not a StatsWidgetEmp!";
    }
}



void MainWindow::refreshEmployeeTable()
{
    QSqlQueryModel *sqlModelEmployee = emp.afficher();
    QStandardItemModel *modelEmployee = new QStandardItemModel(this);
    modelEmployee->setColumnCount(10);
    modelEmployee->setRowCount(sqlModelEmployee->rowCount());

    modelEmployee->setHeaderData(0, Qt::Horizontal, tr("ID"));
    modelEmployee->setHeaderData(1, Qt::Horizontal, tr("Nom"));
    modelEmployee->setHeaderData(2, Qt::Horizontal, tr("Prenom"));
    modelEmployee->setHeaderData(3, Qt::Horizontal, tr("Email"));
    modelEmployee->setHeaderData(4, Qt::Horizontal, tr("Telephone"));
    modelEmployee->setHeaderData(5, Qt::Horizontal, tr("Date de naissance"));
    modelEmployee->setHeaderData(6, Qt::Horizontal, tr("Role"));
    modelEmployee->setHeaderData(7, Qt::Horizontal, tr("Password"));
    modelEmployee->setHeaderData(8, Qt::Horizontal, tr("Supprimer"));
    modelEmployee->setHeaderData(9, Qt::Horizontal, tr("Modifier"));

    for (int row = 0; row < sqlModelEmployee->rowCount(); ++row) {
        modelEmployee->setData(modelEmployee->index(row, 0), sqlModelEmployee->data(sqlModelEmployee->index(row, 0)).toInt());
        modelEmployee->setData(modelEmployee->index(row, 1), sqlModelEmployee->data(sqlModelEmployee->index(row, 1)));
        modelEmployee->setData(modelEmployee->index(row, 2), sqlModelEmployee->data(sqlModelEmployee->index(row, 2)));
        modelEmployee->setData(modelEmployee->index(row, 3), sqlModelEmployee->data(sqlModelEmployee->index(row, 3)));
        modelEmployee->setData(modelEmployee->index(row, 4), sqlModelEmployee->data(sqlModelEmployee->index(row, 4)));
        modelEmployee->setData(modelEmployee->index(row, 5), sqlModelEmployee->data(sqlModelEmployee->index(row, 5)));
        modelEmployee->setData(modelEmployee->index(row, 6), sqlModelEmployee->data(sqlModelEmployee->index(row, 6)));

        // Password: Store in UserRole, display asterisks
        QString password = sqlModelEmployee->data(sqlModelEmployee->index(row, 8)).toString();
        modelEmployee->setData(modelEmployee->index(row, 7), password, Qt::UserRole);
        modelEmployee->setData(modelEmployee->index(row, 7), "****", Qt::DisplayRole);
        modelEmployee->setData(modelEmployee->index(row, 7), false, Qt::UserRole + 1); // Not revealed

        QIcon deleteIcon(":/trash.png");
        QIcon modifyIcon(":/modify.png");

        modelEmployee->setData(modelEmployee->index(row, 8), deleteIcon, Qt::DecorationRole);
        modelEmployee->setData(modelEmployee->index(row, 9), modifyIcon, Qt::DecorationRole);

        modelEmployee->setData(modelEmployee->index(row, 8), "", Qt::DisplayRole);
        modelEmployee->setData(modelEmployee->index(row, 9), "", Qt::DisplayRole);

        modelEmployee->setData(modelEmployee->index(row, 8), Qt::AlignCenter, Qt::TextAlignmentRole);
        modelEmployee->setData(modelEmployee->index(row, 9), Qt::AlignCenter, Qt::TextAlignmentRole);

    }

    ui->tableEmploye->setModel(modelEmployee);


    ui->tableEmploye->setColumnWidth(8, 30);
    ui->tableEmploye->setColumnWidth(9, 30);
    ui->tableEmploye->resizeColumnsToContents();
    refreshStats();
}

void MainWindow::onEmployeeTableClicked(const QModelIndex &index)
{
    int row = index.row();
    int idEmployee = ui->tableEmploye->model()->data(ui->tableEmploye->model()->index(row, 0)).toInt();

    if (index.column() == 7) {
        QAbstractItemModel *model = ui->tableEmploye->model();
        bool isRevealed = model->data(index, Qt::UserRole + 1).toBool();
        QString password = model->data(index, Qt::UserRole).toString();
        model->setData(index, isRevealed ? "****" : password, Qt::DisplayRole);
        model->setData(index, !isRevealed, Qt::UserRole + 1);
    } else if (index.column() == 8) { // Delete column
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Delete",
                                                                  "Are you sure you want to delete this employee?",
                                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            if (emp.supprimer(idEmployee)) {
                refreshEmployeeTable();
                refreshStats();
                QMessageBox::information(this, "Success", "Employee deleted successfully.");
            } else {
                QMessageBox::warning(this, "Error", "Failed to delete employee.");
            }
        }
    } else if (index.column() == 9) { // Modify column
        QAbstractItemModel *modelEmployee = ui->tableEmploye->model();
        QString nom = modelEmployee->data(modelEmployee->index(row, 1)).toString();
        QString prenom = modelEmployee->data(modelEmployee->index(row, 2)).toString();
        QString email = modelEmployee->data(modelEmployee->index(row, 3)).toString();
        int telephone = modelEmployee->data(modelEmployee->index(row, 4)).toInt();
        QString dateN = modelEmployee->data(modelEmployee->index(row, 5)).toString();
        QString role = modelEmployee->data(modelEmployee->index(row, 6)).toString();
        QString password = modelEmployee->data(modelEmployee->index(row, 7), Qt::UserRole).toString();
        newPhotoSelected = false;
        currentPhotoPath = "";

        ui->employeesNavBar->setCurrentIndex(2);

        ui->cinInputM->setText(QString::number(idEmployee));
        ui->nameInputM->setText(nom);
        ui->prenomInputM->setText(prenom);
        ui->emailInputM->setText(email);
        ui->telephoneInputM->setText(QString::number(telephone));
        ui->dateNInputM->setDate(QDate::fromString(dateN, "dd/MM/yyyy"));
        ui->roleInputM->setCurrentText(role);
        ui->mdpInputM->setText(password);
    }
}

void MainWindow::filterEmployeeTable(const QString &searchText)
{
    QSqlQueryModel *sqlModelEmployee = emp.afficher();
    QStandardItemModel *modelEmployee = new QStandardItemModel(this);
    modelEmployee->setColumnCount(10);

    modelEmployee->setHeaderData(0, Qt::Horizontal, tr("ID"));
    modelEmployee->setHeaderData(1, Qt::Horizontal, tr("Nom"));
    modelEmployee->setHeaderData(2, Qt::Horizontal, tr("Prenom"));
    modelEmployee->setHeaderData(3, Qt::Horizontal, tr("Email"));
    modelEmployee->setHeaderData(4, Qt::Horizontal, tr("Telephone"));
    modelEmployee->setHeaderData(5, Qt::Horizontal, tr("Date de naissance"));
    modelEmployee->setHeaderData(6, Qt::Horizontal, tr("Role"));
    modelEmployee->setHeaderData(7, Qt::Horizontal, tr("Password"));
    modelEmployee->setHeaderData(8, Qt::Horizontal, tr("Supprimer"));
    modelEmployee->setHeaderData(9, Qt::Horizontal, tr("Modifier"));

    int rowCount = 0;
    QString searchLower = searchText.toLower();
    for (int row = 0; row < sqlModelEmployee->rowCount(); ++row) {
        QString id = sqlModelEmployee->data(sqlModelEmployee->index(row, 0)).toString();
        QString nom = sqlModelEmployee->data(sqlModelEmployee->index(row, 1)).toString();
        QString prenom = sqlModelEmployee->data(sqlModelEmployee->index(row, 2)).toString();
        QString email = sqlModelEmployee->data(sqlModelEmployee->index(row, 3)).toString();
        QString telephone = sqlModelEmployee->data(sqlModelEmployee->index(row, 4)).toString();
        QString dateN = sqlModelEmployee->data(sqlModelEmployee->index(row, 5)).toString();
        QString role = sqlModelEmployee->data(sqlModelEmployee->index(row, 6)).toString();
        QString password = sqlModelEmployee->data(sqlModelEmployee->index(row, 8)).toString();

        if (id.contains(searchLower, Qt::CaseInsensitive) ||
            nom.contains(searchLower, Qt::CaseInsensitive) ||
            prenom.contains(searchLower, Qt::CaseInsensitive) ||
            email.contains(searchLower, Qt::CaseInsensitive) ||
            telephone.contains(searchLower, Qt::CaseInsensitive) ||
            dateN.contains(searchLower, Qt::CaseInsensitive) ||
            role.contains(searchLower, Qt::CaseInsensitive) ||
            password.contains(searchLower, Qt::CaseInsensitive)) {
            modelEmployee->setRowCount(rowCount + 1);
            modelEmployee->setData(modelEmployee->index(rowCount, 0), id.toInt());
            modelEmployee->setData(modelEmployee->index(rowCount, 1), nom);
            modelEmployee->setData(modelEmployee->index(rowCount, 2), prenom);
            modelEmployee->setData(modelEmployee->index(rowCount, 3), email);
            modelEmployee->setData(modelEmployee->index(rowCount, 4), telephone.toInt());
            modelEmployee->setData(modelEmployee->index(rowCount, 5), dateN);
            modelEmployee->setData(modelEmployee->index(rowCount, 6), role);

            // Password: Store in UserRole, display asterisks
            modelEmployee->setData(modelEmployee->index(rowCount, 7), password, Qt::UserRole);
            modelEmployee->setData(modelEmployee->index(rowCount, 7), "****", Qt::DisplayRole);
            modelEmployee->setData(modelEmployee->index(rowCount, 7), false, Qt::UserRole + 1);

            QIcon deleteIcon(":/trash.png");
            QIcon modifyIcon(":/modify.png");

            modelEmployee->setData(modelEmployee->index(rowCount, 8), deleteIcon, Qt::DecorationRole);
            modelEmployee->setData(modelEmployee->index(rowCount, 9), modifyIcon, Qt::DecorationRole);

            modelEmployee->setData(modelEmployee->index(rowCount, 8), "", Qt::DisplayRole);
            modelEmployee->setData(modelEmployee->index(rowCount, 9), "", Qt::DisplayRole);

            modelEmployee->setData(modelEmployee->index(rowCount, 8), Qt::AlignCenter, Qt::TextAlignmentRole);
            modelEmployee->setData(modelEmployee->index(rowCount, 9), Qt::AlignCenter, Qt::TextAlignmentRole);

            rowCount++;
        }
    }

    ui->tableEmploye->setModel(modelEmployee);

    ui->tableEmploye->setColumnWidth(8, 30);
    ui->tableEmploye->setColumnWidth(9, 30);
    ui->tableEmploye->resizeColumnsToContents();
}

void MainWindow::on_ajouterEmpBD_clicked()
{
    // Get the form variables
    QString idStr = ui->cinInput->text();
    QString nomEmp = ui->nameInput->text();
    QString prenomEmp = ui->prenomInput->text();
    QString emailEmp = ui->emailInput->text();
    QString telephoneStr = ui->telephoneInput->text();
    QString dateN = ui->dateNInput->text();
    QString roleEmp = ui->roleInput->currentText();
    QString password = ui->mdpInput->text();
    std::vector<unsigned char> photo;
    QString fileName = ui->photoInput->text();
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray imageData = file.readAll();
            photo.assign(imageData.begin(), imageData.end());
            file.close();
        } else {
            QMessageBox::warning(this, "Erreur de saisie", "Failed to open image file.");
            return;
        }
    }

    // Validation - Stop at the first error
    bool idOk;
    int id_employee = idStr.toInt(&idOk);
    if (!idOk || id_employee <= 0 || idStr.length() != 8) {
        QMessageBox::warning(this, "Erreur de saisie", "ID est un entier de 8 chiffres.");
        return;
    }

    QRegularExpression nameRegex("^[A-Za-zÀ-ÿ -]+$");
    if (nomEmp.isEmpty() || !nameRegex.match(nomEmp).hasMatch()) {
        QMessageBox::warning(this, "Erreur de saisie", "Le nom ne doit pas être vide et ne doit contenir que des lettres.");
        return;
    }

    if (prenomEmp.isEmpty() || !nameRegex.match(prenomEmp).hasMatch()) {
        QMessageBox::warning(this, "Erreur de saisie", "Le prénom ne doit pas être vide et ne doit contenir que des lettres.");
        return;
    }

    QRegularExpression emailRegex("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
    if (!emailRegex.match(emailEmp).hasMatch()) {
        QMessageBox::warning(this, "Erreur de saisie", "Format d'email invalide.");
        return;
    }

    bool telOk;
    int telephoneEmp = telephoneStr.toInt(&telOk);
    if (!telOk || telephoneEmp <= 0 || telephoneStr.length() != 8) {
        QMessageBox::warning(this, "Erreur de saisie", "Le téléphone doit être un nombre positif de 8 chiffres.");
        return;
    }

    if (roleEmp.isEmpty()) {
        QMessageBox::warning(this, "Erreur de saisie", "Le rôle doit être sélectionné.");
        return;
    }

    if (password.length() < 6) {
        QMessageBox::warning(this, "Erreur de saisie", "Le mot de passe doit contenir au moins 6 caractères.");
        return;
    }

    // If we reach here, all validations passed
    // Create an employee object
    Employe e(id_employee, nomEmp.toStdString(), prenomEmp.toStdString(), emailEmp.toStdString(),
              telephoneEmp, dateN.toStdString(), roleEmp.toStdString(), photo, password.toStdString());

    // Add the employee to the database
    bool test = e.ajouter();
    if (test) {
        ui->MessageForme->setText("Employé ajouté avec succès ✅");
        refreshEmployeeTable();
        refreshStats();
    } else {
        ui->MessageForme->setText("Erreur : Employé non ajouté ❎");
    }
}

void MainWindow::on_modifierEmpBD_clicked()
{
    // Get form variables
    QString idStr = ui->cinInputM->text();
    QString nomEmp = ui->nameInputM->text();
    QString prenomEmp = ui->prenomInputM->text();
    QString emailEmp = ui->emailInputM->text();
    QString telephoneStr = ui->telephoneInputM->text();
    QString dateN = ui->dateNInputM->text();
    QString roleEmp = ui->roleInputM->currentText();
    QString password = ui->mdpInputM->text();

    // Initialize photo vector
    std::vector<unsigned char> photo;

    // Check for new photo
    qDebug() << "currentPhotoPath:" << currentPhotoPath << ", newPhotoSelected:" << newPhotoSelected;

    if (newPhotoSelected && !currentPhotoPath.isEmpty()) {
        QFile file(currentPhotoPath);
        if (file.exists() && file.open(QIODevice::ReadOnly)) {
            QByteArray imageData = file.readAll();
            photo.assign(imageData.begin(), imageData.end());
            file.close();
            qDebug() << "New photo loaded, size:" << photo.size();
        } else {
            QMessageBox::warning(this, "Erreur", "Cannot open selected file: " + currentPhotoPath);
            qDebug() << "Failed to open file:" << currentPhotoPath;
            return;
        }
    } else {
        qDebug() << "No new photo selected, will use existing DB photo";
    }

    // Validation
    bool idOk;
    int id_employee = idStr.toInt(&idOk);
    if (!idOk || id_employee <= 0 || idStr.length() != 8) {
        QMessageBox::warning(this, "Erreur de saisie", "ID must be an 8-digit positive number.");
        return;
    }

    QRegularExpression nameRegex("^[A-Za-zÀ-ÿ -]+$");
    if (nomEmp.isEmpty() || !nameRegex.match(nomEmp).hasMatch()) {
        QMessageBox::warning(this, "Erreur de saisie", "Nom must not be empty and contain only letters.");
        return;
    }

    if (prenomEmp.isEmpty() || !nameRegex.match(prenomEmp).hasMatch()) {
        QMessageBox::warning(this, "Erreur de saisie", "Prenom must not be empty and contain only letters.");
        return;
    }

    QRegularExpression emailRegex("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
    if (!emailRegex.match(emailEmp).hasMatch()) {
        QMessageBox::warning(this, "Erreur de saisie", "Invalid email format.");
        return;
    }

    bool telOk;
    int telephoneEmp = telephoneStr.toInt(&telOk);
    if (!telOk || telephoneEmp <= 0 || telephoneStr.length() != 8) {
        QMessageBox::warning(this, "Erreur de saisie", "Telephone must be an 8-digit positive number.");
        return;
    }

    if (roleEmp.isEmpty()) {
        QMessageBox::warning(this, "Erreur de saisie", "Role must be selected.");
        return;
    }

    if (password.length() < 6) {
        QMessageBox::warning(this, "Erreur de saisie", "Password must be at least 6 characters long.");
        return;
    }

    // Create employee object
    Employe e(id_employee, nomEmp.toStdString(), prenomEmp.toStdString(), emailEmp.toStdString(),
              telephoneEmp, dateN.toStdString(), roleEmp.toStdString(), photo, password.toStdString());

    // Load existing photo if no new one
    if (!newPhotoSelected) {
        QSqlQuery query;
        query.prepare("SELECT photo FROM employe WHERE id_employe = :id_employee");
        query.bindValue(":id_employee", id_employee);
        if (query.exec() && query.next()) {
            QByteArray photoData = query.value(0).toByteArray();
            if (!photoData.isEmpty()) {
                photo.assign(photoData.begin(), photoData.end());
                e.setPhoto(photo);
                qDebug() << "Loaded existing photo from DB, size:" << photo.size();
            } else {
                qDebug() << "No photo exists in DB for ID:" << id_employee;
            }
        } else {
            QMessageBox::warning(this, "Erreur", "Failed to fetch DB photo: ");

            return;
        }
    }

    // Modify employee
    bool test = e.modifier();
    if (test) {
        ui->MessageFormeM->setText("Employé modifié avec succès ✅");
        refreshEmployeeTable();
        refreshStats();
        qDebug() << "Employee modified successfully";
        newPhotoSelected = false;
        currentPhotoPath = "";
    } else {
        ui->MessageFormeM->setText("Erreur : Employé non modifié ❎");
        qDebug() << "Failed to modify employee";
    }
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

// Etablissements Navbar
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

void MainWindow::on_photoInput_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "/home", tr("Image Files (*.png *.jpg *.bmp)"));
    ui->photoInput->setText(fileName);
}

void MainWindow::on_champRecherche_textChanged(const QString &text)
{
    filterEmployeeTable(text);
}


void MainWindow::on_pdfEmp_clicked()
{
    QModelIndexList selectedRows = ui->tableEmploye->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select an employee from the table to generate a PDF.");
        return;
    }

    int row = selectedRows[0].row();
    QAbstractItemModel *model = ui->tableEmploye->model();
    int idEmployee = model->data(model->index(row, 0)).toInt();
    QString nom = model->data(model->index(row, 1)).toString();
    QString prenom = model->data(model->index(row, 2)).toString();
    QString email = model->data(model->index(row, 3)).toString();
    int telephone = model->data(model->index(row, 4)).toInt();
    QString dateNRaw = model->data(model->index(row, 5)).toString();
    QString role = model->data(model->index(row, 6)).toString();
    QString password = model->data(model->index(row, 7), Qt::UserRole).toString();

    // Parse the date and strip time
    QString dateN;
    if (dateNRaw.contains("T")) {
        QDateTime dateTime = QDateTime::fromString(dateNRaw, Qt::ISODate);
        dateN = dateTime.isValid() ? dateTime.date().toString("MM/dd/yyyy") : dateNRaw;
    } else if (dateNRaw.contains("-") && !dateNRaw.contains(":")) {
        QDate date = QDate::fromString(dateNRaw, "yyyy-MM-dd");
        if (!date.isValid()) {
            date = QDate::fromString(dateNRaw, "dd-MMM-yy");
            if (date.year() < 1970 && date.isValid()) {
                date = date.addYears(100);
            }
        }
        dateN = date.isValid() ? date.toString("MM/dd/yyyy") : dateNRaw;
    } else {
        dateN = dateNRaw;
    }
    qDebug() << "Raw date from table:" << dateNRaw << "Formatted date:" << dateN;

    QSqlQuery query;
    query.prepare("SELECT photo FROM employe WHERE id_employe = :id_employee");
    query.bindValue(":id_employee", idEmployee);
    if (!query.exec() || !query.next()) {
        QMessageBox::warning(this, "Error", "Failed to retrieve employee photo.");
        return;
    }

    QByteArray photoData = query.value(0).toByteArray();
    QImage photo;
    if (!photoData.isEmpty() && !photo.loadFromData(photoData)) {
        QMessageBox::warning(this, "Error", "Failed to load photo data.");
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save PDF"),
                                                    QString("%1_%2_employee.pdf").arg(nom).arg(prenom),
                                                    tr("PDF Files (*.pdf)"));
    if (fileName.isEmpty()) return;

    QPdfWriter pdfWriter(fileName);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setResolution(300);

    QPainter painter(&pdfWriter);
    painter.setRenderHint(QPainter::Antialiasing);

    const int pageWidth = pdfWriter.width();
    const int pageHeight = pdfWriter.height();
    const int margin = 500;
    const int lineSpacing = 250;
    const int maxPhotoWidth = 450;
    const int maxPhotoHeight = 450;

    painter.setBrush(QBrush(QColor(200, 220, 255)));
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, 0, pageWidth, 800);

    QFont titleFont("Arial", 18, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(Qt::black);
    painter.drawText(margin, 400, "Employee Profile");

    if (!photo.isNull()) {
        int photoWidth = photo.width();
        int photoHeight = photo.height();
        if (photoWidth > maxPhotoWidth) {
            photoWidth = maxPhotoWidth;
            photoHeight = photo.height() * maxPhotoWidth / photo.width();
        }
        if (photoHeight > maxPhotoHeight) {
            photoHeight = maxPhotoHeight;
            photoWidth = photo.width() * maxPhotoHeight / photo.height();
        }
        int photoX = pageWidth - photoWidth;
        int photoY = 0;
        QRect photoRect(photoX, photoY, photoWidth, photoHeight);
        painter.drawImage(photoRect, photo);
        painter.setPen(QPen(Qt::black, 10));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(photoRect);
    } else {
        painter.setFont(QFont("Arial", 12));
        painter.drawText(pageWidth - 200, 100, "No photo");
    }

    int yPos = 800;
    QFont labelFont("Arial", 12, QFont::Bold);
    QFont valueFont("Arial", 12);
    const int labelX = margin + 50;
    const int valueX = margin + 600;
    const int maxValueWidth = pageWidth - valueX - margin;

    painter.setPen(QPen(Qt::gray, 5));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(margin, yPos - lineSpacing / 2, pageWidth - 2 * margin, 8 * lineSpacing + 50);

    painter.setPen(Qt::black);

    painter.setFont(labelFont);
    painter.drawText(labelX, yPos, "ID:");
    painter.setFont(valueFont);
    painter.drawText(valueX, yPos, QString("%1").arg(idEmployee));
    yPos += lineSpacing;

    painter.setFont(labelFont);
    painter.drawText(labelX, yPos, "Nom:");
    painter.setFont(valueFont);
    QString nomText = nom;
    if (painter.fontMetrics().boundingRect(nomText).width() > maxValueWidth) {
        nomText = painter.fontMetrics().elidedText(nom, Qt::ElideRight, maxValueWidth);
    }
    painter.drawText(valueX, yPos, nomText);
    yPos += lineSpacing;

    painter.setFont(labelFont);
    painter.drawText(labelX, yPos, "Prenom:");
    painter.setFont(valueFont);
    QString prenomText = prenom;
    if (painter.fontMetrics().boundingRect(prenomText).width() > maxValueWidth) {
        prenomText = painter.fontMetrics().elidedText(prenom, Qt::ElideRight, maxValueWidth);
    }
    painter.drawText(valueX, yPos, prenomText);
    yPos += lineSpacing;

    painter.setFont(labelFont);
    painter.drawText(labelX, yPos, "Email:");
    painter.setFont(valueFont);
    QString emailText = email;
    if (painter.fontMetrics().boundingRect(emailText).width() > maxValueWidth) {
        emailText = painter.fontMetrics().elidedText(email, Qt::ElideRight, maxValueWidth);
    }
    painter.drawText(valueX, yPos, emailText);
    yPos += lineSpacing;

    painter.setFont(labelFont);
    painter.drawText(labelX, yPos, "Telephone:");
    painter.setFont(valueFont);
    painter.drawText(valueX, yPos, QString("%1").arg(telephone));
    yPos += lineSpacing;

    painter.setFont(labelFont);
    painter.drawText(labelX, yPos, "Date de naissance:");
    painter.setFont(valueFont);
    QString dateNText = dateN;
    if (painter.fontMetrics().boundingRect(dateNText).width() > maxValueWidth) {
        dateNText = painter.fontMetrics().elidedText(dateN, Qt::ElideRight, maxValueWidth);
    }
    painter.drawText(valueX, yPos, dateNText);
    yPos += lineSpacing;

    painter.setFont(labelFont);
    painter.drawText(labelX, yPos, "Role:");
    painter.setFont(valueFont);
    QString roleText = role;
    if (painter.fontMetrics().boundingRect(roleText).width() > maxValueWidth) {
        roleText = painter.fontMetrics().elidedText(role, Qt::ElideRight, maxValueWidth);
    }
    painter.drawText(valueX, yPos, roleText);
    yPos += lineSpacing;

    painter.setFont(labelFont);
    painter.drawText(labelX, yPos, "Password:");
    painter.setFont(valueFont);
    QString passwordText = password;
    if (painter.fontMetrics().boundingRect(passwordText).width() > maxValueWidth) {
        passwordText = painter.fontMetrics().elidedText(password, Qt::ElideRight, maxValueWidth);
    }
    painter.drawText(valueX, yPos, passwordText);
    yPos += lineSpacing;

    QFont footerFont("Arial", 10, QFont::Light);
    painter.setFont(footerFont);
    painter.setPen(Qt::gray);
    painter.drawText(margin, pageHeight - margin / 2,
                     QString("Generated on %1").arg(QDate::currentDate().toString("dd/MM/yyyy")));

    painter.end();
    QMessageBox::information(this, "Success", "PDF generated successfully!");
}




void MainWindow::on_photoInputM_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "/home", tr("Image Files (*.png *.jpg *.bmp)"));
    if (!fileName.isEmpty() && QFile::exists(fileName)) {
        currentPhotoPath = fileName;
        newPhotoSelected = true;
        qDebug() << "New photo selected:" << fileName;
    } else {
        currentPhotoPath = "";
        newPhotoSelected = false;
        qDebug() << "No valid photo selected";
    }
}

void MainWindow::on_LOGINBTN_clicked()
{
    // Get input values from the login form
    QString cinLogin = ui->CINLOGIN->text(); // Assuming the QLineEdit is named cinLoginInput
    QString mdpLogin = ui->MDPLOGIN->text(); // Assuming the QLineEdit is named mdpLoginInput

    // Basic input validation
    if (cinLogin.isEmpty() || mdpLogin.isEmpty()) {
        QMessageBox::warning(this, "Erreur de connexion", "Veuillez entrer un CIN et un mot de passe.");
        return;
    }

    bool cinOk;
    int id_employee = cinLogin.toInt(&cinOk);
    if (!cinOk || cinLogin.length() != 8) {
        QMessageBox::warning(this, "Erreur de connexion", "Le CIN doit être un numéro de 8 chiffres.");
        return;
    }

    // Query the database to verify credentials
    QSqlQuery query;
    query.prepare("SELECT id_employe, password FROM employe WHERE id_employe = :id_employee AND password = :password");
    query.bindValue(":id_employee", id_employee);
    query.bindValue(":password", mdpLogin);

    if (query.exec() && query.next()) {
        // Login successful
        QMessageBox::information(this, "Connexion réussie", "Bienvenue !");
        ui->login_app->setCurrentIndex(1);
        ui->CINLOGIN->clear();
        ui->MDPLOGIN->clear();
    } else {
        // Login failed
        QMessageBox::warning(this, "Erreur de connexion", "CIN ou mot de passe incorrect.");
    }
}

void MainWindow::on_LOGINFACIAL_clicked()
{
    if (!faceRecognitionActive) {
        // Start face recognition
        if (!cap.isOpened()) {
            cap.open(0);
            if (!cap.isOpened()) {
                QMessageBox::critical(this, "Error", "Could not open camera.");
                return;
            }
        }
        timer->start(30); // ~30 FPS
        ui->LOGINFACIAL->setText("STOP FACIAL");
        ui->cameraLabel->show();
        faceRecognitionActive = true;
        consecutiveDetections = 0; // Reset counter when starting
    } else {
        // Stop face recognition
        timer->stop();
        if (cap.isOpened()) {
            cap.release();
        }
        ui->cameraLabel->clear();
        ui->cameraLabel->hide();
        ui->LOGINFACIAL->setText("LOGINFACIAL");
        faceRecognitionActive = false;
        consecutiveDetections = 0; // Reset counter when stopping
    }
}

void MainWindow::updateFrame()
{
    if (!cap.isOpened()) return;

    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) {
        qDebug() << "Empty frame captured";
        return;
    }

    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(gray, faces, 1.1, 5, 0, cv::Size(30, 30));

    if (faceRecognitionActive) {
        bool azizDetected = false;
        for (const auto& face : faces) {
            cv::Mat faceROI = gray(face);
            cv::resize(faceROI, faceROI, cv::Size(100, 100));
            int label = -1;
            double confidence = 0.0;
            recognizer->predict(faceROI, label, confidence);
            qDebug() << "Predicted Label:" << label << "Confidence:" << confidence;
            cv::rectangle(frame, face, cv::Scalar(0, 255, 0), 2);
            std::string name = (label >= 0 && label < user_names.size() && confidence < 80)
                                   ? user_names[label] : "Unknown";
            cv::putText(frame, name + " (" + std::to_string((int)confidence) + ")",
                        cv::Point(face.x, face.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(0, 255, 0), 2);
            if (name == "Aziz") {
                azizDetected = true;
                consecutiveDetections++;
                qDebug() << "Consecutive Detections:" << consecutiveDetections;
            } else {
                consecutiveDetections = 0;
            }
            if (consecutiveDetections >= 60) {
                QMessageBox::information(this, "Connexion réussie", "Bienvenue, Aziz !");
                ui->login_app->setCurrentIndex(1);
                timer->stop();
                cap.release();
                ui->cameraLabel->clear();
                ui->cameraLabel->hide();
                ui->LOGINFACIAL->setText("LOGINFACIAL");
                faceRecognitionActive = false;
                consecutiveDetections = 0;
                break;
            }
        }
        if (!azizDetected) {
            consecutiveDetections = 0;
            qDebug() << "Reset Consecutive Detections: No Aziz detected";
        }
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
        QImage qimg(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
        ui->cameraLabel->setPixmap(QPixmap::fromImage(qimg));
    } else if (emotionRecognitionActive) {
        cv::resize(frame, frame, cv::Size(640, 480));
        cv::equalizeHist(gray, gray);
        QString currentSentiment = "Neutral";

        for (const auto& face : faces) {
            cv::rectangle(frame, face, cv::Scalar(0, 255, 0), 2);
            cv::Mat faceROI = gray(face);
            std::vector<cv::Rect> smiles;
            smileCascade.detectMultiScale(faceROI, smiles, 1.7, 25, 0, cv::Size(20, 20));

            if (!smiles.empty()) {
                currentSentiment = "Happy";
                for (const auto& smile : smiles) {
                    cv::rectangle(frame, cv::Point(face.x + smile.x, face.y + smile.y),
                                  cv::Point(face.x + smile.x + smile.width, face.y + smile.y + smile.height),
                                  cv::Scalar(255, 0, 0), 2);
                }
            }
        }

        if (currentSentiment == "Neutral") {
            neutralFrameCount++;
            happyFrameCount = 0;
            if (neutralFrameCount >= 5 && lastSentiment != "Neutral") {
                lastSentiment = "Neutral";
                displayCheerUpContent();
                qDebug() << "Neutral detected, quote set to:" << cheerUpQuote;
            }
        } else {
            neutralFrameCount = 0;
            happyFrameCount++;
            if (lastSentiment != "Happy") {
                lastSentiment = "Happy";
                cheerUpQuote = "";
                qDebug() << "Happy detected, cleared quote";
            }
        }

        // Stop after 3 seconds (90 frames at ~30 FPS) of happy
        if (happyFrameCount >= 30) {
            qDebug() << "3 seconds of happy detected, stopping emotion recognition";
            timer->stop();
            if (cap.isOpened()) {
                cap.release();
            }
            ui->emotionLabel->clear();
            ui->emotionLabel->setText("Emotion Feed");
            emotionRecognitionActive = false;
            neutralFrameCount = 0;
            happyFrameCount = 0;
            lastSentiment = "None";
            cheerUpQuote = "";
            ui->emotionLabel->hide();
            ui->facialEmotion->setText("FACIAL EMOTION");
            toggleTimer->start(60000); // Resume auto-toggle
            return; // Exit to avoid updating label after stopping
        }

        cv::putText(frame, "Sentiment: " + lastSentiment.toStdString(),
                    cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(0, 255, 255), 2);
        if (!cheerUpQuote.isEmpty()) {
            cv::putText(frame, cheerUpQuote.toStdString(),
                        cv::Point(10, frame.rows - 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 0), 2);
        }
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
        QImage qimg(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
        ui->emotionLabel->setPixmap(QPixmap::fromImage(qimg));
    }
}

//SENTIMENTS

void MainWindow::displayCheerUpContent()
{
    if (cheerMessages.isEmpty()) {
        cheerUpQuote = "";
        return;
    }

    int index = QRandomGenerator::global()->bounded(cheerMessages.size());
    cheerUpQuote = cheerMessages[index];
}

void MainWindow::on_facialEmotion_clicked()
{
    if (!emotionRecognitionActive) {
        if (!cap.isOpened()) {
            cap.open(0);
            if (!cap.isOpened()) {
                QMessageBox::critical(this, "Error", "Could not open camera.");
                return;
            }
        }
        timer->start(33);
        ui->facialEmotion->setText("STOP EMOTION");
        ui->emotionLabel->show();
        emotionRecognitionActive = true;
        if (faceRecognitionActive) {
            faceRecognitionActive = false;
            ui->LOGINFACIAL->setText("LOGINFACIAL");
            ui->cameraLabel->clear();
            ui->cameraLabel->hide();
            consecutiveDetections = 0;
        }
        neutralFrameCount = 0;
        happyFrameCount = 0;
        lastSentiment = "None";
        cheerUpQuote = "";
        toggleTimer->stop();
        qDebug() << "Emotion recognition started manually";
    } else {
        timer->stop();
        if (cap.isOpened()) {
            cap.release();
        }
        ui->emotionLabel->clear();
        ui->emotionLabel->setText("Emotion Feed");
        ui->facialEmotion->setText("FACIAL EMOTION");
        emotionRecognitionActive = false;
        neutralFrameCount = 0;
        happyFrameCount = 0;
        lastSentiment = "None";
        cheerUpQuote = "";
        toggleTimer->start(60000);
        ui->emotionLabel->hide();
        qDebug() << "Emotion recognition stopped manually";
    }
}
void MainWindow::toggleEmotionRecognition()
{
    if (!emotionRecognitionActive) {
        if (!cap.isOpened()) {
            cap.open(0);
            if (!cap.isOpened()) {
                qDebug() << "Auto-toggle: Could not open camera.";
                return;
            }
        }
        timer->start(33);
        ui->emotionLabel->show();
        emotionRecognitionActive = true;
        if (faceRecognitionActive) {
            faceRecognitionActive = false;
            ui->LOGINFACIAL->setText("LOGINFACIAL");
            ui->cameraLabel->clear();
            ui->cameraLabel->hide();
            consecutiveDetections = 0;
        }
        neutralFrameCount = 0;
        happyFrameCount = 0;
        lastSentiment = "None";
        cheerUpQuote = "";
        qDebug() << "Auto-toggle: Emotion recognition started";
    } else {
        timer->stop();
        if (cap.isOpened()) {
            cap.release();
        }
        ui->emotionLabel->clear();
        ui->emotionLabel->setText("Emotion Feed");
        emotionRecognitionActive = false;
        neutralFrameCount = 0;
        happyFrameCount = 0;
        lastSentiment = "None";
        cheerUpQuote = "";
        ui->emotionLabel->hide();
        qDebug() << "Auto-toggle: Emotion recognition stopped";
    }
}
