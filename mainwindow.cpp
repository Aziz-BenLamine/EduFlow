#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QRegularExpression>
#include <QDate>

#include "employe.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    refreshEmployeeTable();
    connect(ui->tableEmploye, &QTableView::clicked, this, &MainWindow::onEmployeeTableClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::refreshEmployeeTable()
{
    // Get the original QSqlQueryModel from afficher()
    QSqlQueryModel *sqlModelEmployee = emp.afficher();

    // Create a new editable QStandardItemModel for employees
    QStandardItemModel *modelEmployee = new QStandardItemModel(this);
    modelEmployee->setColumnCount(10); // 8 columns (excluding photo) + 2 for Delete/Modify
    modelEmployee->setRowCount(sqlModelEmployee->rowCount());

    // Define headers explicitly, omitting photo
    modelEmployee->setHeaderData(0, Qt::Horizontal, tr("ID"));
    modelEmployee->setHeaderData(1, Qt::Horizontal, tr("Nom"));
    modelEmployee->setHeaderData(2, Qt::Horizontal, tr("Prenom"));
    modelEmployee->setHeaderData(3, Qt::Horizontal, tr("Email"));
    modelEmployee->setHeaderData(4, Qt::Horizontal, tr("Telephone"));
    modelEmployee->setHeaderData(5, Qt::Horizontal, tr("Date de naissance"));
    modelEmployee->setHeaderData(6, Qt::Horizontal, tr("Role"));
    modelEmployee->setHeaderData(7, Qt::Horizontal, tr("Password"));
    modelEmployee->setHeaderData(8, Qt::Horizontal, tr("Delete"));
    modelEmployee->setHeaderData(9, Qt::Horizontal, tr("Modify"));

    // Copy data from sqlModelEmployee to the new model, skipping photo (column 7 in DB)
    for (int row = 0; row < sqlModelEmployee->rowCount(); ++row) {
        // Column mappings: DB column -> Model column
        // 0: id_employe -> 0
        modelEmployee->setData(modelEmployee->index(row, 0),
                               sqlModelEmployee->data(sqlModelEmployee->index(row, 0)).toInt());
        // 1: nomEmp -> 1
        modelEmployee->setData(modelEmployee->index(row, 1),
                               sqlModelEmployee->data(sqlModelEmployee->index(row, 1)));
        // 2: prenomEmp -> 2
        modelEmployee->setData(modelEmployee->index(row, 2),
                               sqlModelEmployee->data(sqlModelEmployee->index(row, 2)));
        // 3: email -> 3
        modelEmployee->setData(modelEmployee->index(row, 3),
                               sqlModelEmployee->data(sqlModelEmployee->index(row, 3)));
        // 4: telephone -> 4
        modelEmployee->setData(modelEmployee->index(row, 4),
                               sqlModelEmployee->data(sqlModelEmployee->index(row, 4)));
        // 5: dateN -> 5
        modelEmployee->setData(modelEmployee->index(row, 5),
                               sqlModelEmployee->data(sqlModelEmployee->index(row, 5)));
        // 6: role -> 6
        modelEmployee->setData(modelEmployee->index(row, 6),
                               sqlModelEmployee->data(sqlModelEmployee->index(row, 6)));
        // 8: password -> 7 (skip 7: photo)
        modelEmployee->setData(modelEmployee->index(row, 7),
                               sqlModelEmployee->data(sqlModelEmployee->index(row, 8)));

        // Add Delete and Modify text
        modelEmployee->setData(modelEmployee->index(row, 8), "[Delete]");
        modelEmployee->setData(modelEmployee->index(row, 9), "[Modify]");
    }

    // Set the new model to the employee table
    ui->tableEmploye->setModel(modelEmployee);

    // Adjust column widths
    ui->tableEmploye->resizeColumnsToContents();
}

void MainWindow::onEmployeeTableClicked(const QModelIndex &index)
{
    int row = index.row();
    int idEmployee = ui->tableEmploye->model()->data(ui->tableEmploye->model()->index(row, 0)).toInt();

    if (index.column() == 8) { // Delete column
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Delete",
                                                                  "Are you sure you want to delete this employee?",
                                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            if (emp.supprimer(idEmployee)) {
                refreshEmployeeTable(); // Refresh employee table
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
        QString password = modelEmployee->data(modelEmployee->index(row, 7)).toString();

        // Switch to the modify form (index 2)
        ui->employeesNavBar->setCurrentIndex(2);

        // Populate the modify form fields
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
            QMessageBox::warning(this, "Error", "Failed to open image file.");
            return;
        }
    }

    // Validation
    QString errorMsg;

    // ID validation
    bool idOk;
    int id_employee = idStr.toInt(&idOk);
    if (!idOk || id_employee <= 0 || idStr.length() != 8) {
        errorMsg = "ID est un entier de 8 chiffres.";
    }

    // Nom validation
    QRegularExpression nameRegex("^[A-Za-zÀ-ÿ -]+$"); // Letters, spaces, hyphens, accented chars
    if (nomEmp.isEmpty() || !nameRegex.match(nomEmp).hasMatch()) {
        errorMsg += "\nLe nom ne doit pas être vide et ne doit contenir que des lettres.";
    }

    // Prenom validation
    if (prenomEmp.isEmpty() || !nameRegex.match(prenomEmp).hasMatch()) {
        errorMsg += "\nLe prénom ne doit pas être vide et ne doit contenir que des lettres.";
    }

    // Email validation
    QRegularExpression emailRegex("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
    if (!emailRegex.match(emailEmp).hasMatch()) {
        errorMsg += "\nFormat d'email invalide.";
    }

    // Telephone validation
    bool telOk;
    int telephoneEmp = telephoneStr.toInt(&telOk);
    if (!telOk || telephoneEmp <= 0 || telephoneStr.length() != 8) {
        errorMsg += "\nLe téléphone doit être un nombre positif de 8 chiffres.";
    }

    // Role validation (assuming combo box ensures non-empty)
    if (roleEmp.isEmpty()) {
        errorMsg += "\nLe rôle doit être sélectionné.";
    }

    // Password validation
    if (password.length() < 6) {
        errorMsg += "\nLe mot de passe doit contenir au moins 6 caractères.";
    }

    // Check if there are errors
    if (!errorMsg.isEmpty()) {
        ui->MessageForme->setText("Erreur : Employé non ajouté ❎\n" + errorMsg.trimmed());
        return; // Stop execution if validation fails
    }

    // Create an employee object
    Employe e(id_employee, nomEmp.toStdString(), prenomEmp.toStdString(), emailEmp.toStdString(),
              telephoneEmp, dateN.toStdString(), roleEmp.toStdString(), photo, password.toStdString());

    // Add the employee to the database
    bool test = e.ajouter();
    if (test) {
        ui->MessageForme->setText("Employé ajouté avec succès ✅");
        QMessageBox::information(this, tr("Employee added"),
                                 tr("Employee added successfully.\nClick Cancel to exit."), QMessageBox::Cancel);
        refreshEmployeeTable();
    } else {
        ui->MessageForme->setText("Erreur : Employé non ajouté ❎");
        QMessageBox::critical(this, tr("Employee not added"),
                              tr("Employee not added.\nClick Cancel to exit."), QMessageBox::Cancel);
    }
}

void MainWindow::on_modifierEmpBD_clicked()
{
    // Get the form variables from modify form
    QString idStr = ui->cinInputM->text();
    QString nomEmp = ui->nameInputM->text();
    QString prenomEmp = ui->prenomInputM->text();
    QString emailEmp = ui->emailInputM->text();
    QString telephoneStr = ui->telephoneInputM->text();
    QString dateN = ui->dateNInputM->text();
    QString roleEmp = ui->roleInputM->currentText();
    QString password = ui->mdpInputM->text();

    // Load photo (assuming there's a ui->photoInputM for the modify form)
    std::vector<unsigned char> photo;
    /*QString fileName = ui->photoInputM->text(); // Adjust if the widget name differs
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray imageData = file.readAll();
            photo.assign(imageData.begin(), imageData.end());
            file.close();
        } else {
            QMessageBox::warning(this, "Error", "Failed to open image file.");
            return;
        }
    }
    */
    // Validation
    QString errorMsg;

    // ID validation
    bool idOk;
    int id_employee = idStr.toInt(&idOk);
    if (!idOk || id_employee <= 0 || idStr.length() != 8) {
        errorMsg = "ID must be an 8-digit positive number.";
    }

    // Nom validation
    QRegularExpression nameRegex("^[A-Za-zÀ-ÿ -]+$");
    if (nomEmp.isEmpty() || !nameRegex.match(nomEmp).hasMatch()) {
        errorMsg += "\nNom must not be empty and contain only letters.";
    }

    // Prenom validation
    if (prenomEmp.isEmpty() || !nameRegex.match(prenomEmp).hasMatch()) {
        errorMsg += "\nPrenom must not be empty and contain only letters.";
    }

    // Email validation
    QRegularExpression emailRegex("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
    if (!emailRegex.match(emailEmp).hasMatch()) {
        errorMsg += "\nInvalid email format.";
    }

    // Telephone validation
    bool telOk;
    int telephoneEmp = telephoneStr.toInt(&telOk);
    if (!telOk || telephoneEmp <= 0 || telephoneStr.length() != 8) {
        errorMsg += "\nTelephone must be an 8-digit positive number.";
    }

    // Role validation
    if (roleEmp.isEmpty()) {
        errorMsg += "\nRole must be selected.";
    }

    // Password validation
    if (password.length() < 6) {
        errorMsg += "\nPassword must be at least 6 characters long.";
    }

    // Check if there are errors
    if (!errorMsg.isEmpty()) {
        ui->MessageFormeM->setText("Erreur : Employé non modifié ❎\n" + errorMsg.trimmed());
        return; // Stop execution if validation fails
    }

    // Create an employee object with updated values
    Employe e(id_employee, nomEmp.toStdString(), prenomEmp.toStdString(), emailEmp.toStdString(),
              telephoneEmp, dateN.toStdString(), roleEmp.toStdString(), photo, password.toStdString());

    // Modify the employee in the database
    bool test = e.modifier();
    if (test) {
        ui->MessageFormeM->setText("Employé modifié avec succès ✅");
        QMessageBox::information(this, tr("Employee modified"),
                                 tr("Employee modified successfully.\nClick Cancel to exit."), QMessageBox::Cancel);
        refreshEmployeeTable();
    } else {
        ui->MessageFormeM->setText("Erreur : Employé non modifié ❎");
        QMessageBox::critical(this, tr("Employee not modified"),
                              tr("Employee not modified.\nClick Cancel to exit."), QMessageBox::Cancel);
    }
}

void MainWindow::filterEmployeeTable(const QString &searchText)
{
    // Get the original data from the database
    QSqlQueryModel *sqlModelEmployee = emp.afficher();

    // Create a new editable QStandardItemModel for employees
    QStandardItemModel *modelEmployee = new QStandardItemModel(this);
    modelEmployee->setColumnCount(10); // 8 columns (excluding photo) + 2 for Delete/Modify

    // Set headers (same as refreshEmployeeTable)
    modelEmployee->setHeaderData(0, Qt::Horizontal, tr("ID"));
    modelEmployee->setHeaderData(1, Qt::Horizontal, tr("Nom"));
    modelEmployee->setHeaderData(2, Qt::Horizontal, tr("Prenom"));
    modelEmployee->setHeaderData(3, Qt::Horizontal, tr("Email"));
    modelEmployee->setHeaderData(4, Qt::Horizontal, tr("Telephone"));
    modelEmployee->setHeaderData(5, Qt::Horizontal, tr("Date de naissance"));
    modelEmployee->setHeaderData(6, Qt::Horizontal, tr("Role"));
    modelEmployee->setHeaderData(7, Qt::Horizontal, tr("Password"));
    modelEmployee->setHeaderData(8, Qt::Horizontal, tr("Delete"));
    modelEmployee->setHeaderData(9, Qt::Horizontal, tr("Modify"));

    // Filter rows based on search text
    int rowCount = 0;
    QString searchLower = searchText.toLower(); // Case-insensitive search
    for (int row = 0; row < sqlModelEmployee->rowCount(); ++row) {
        QString id = sqlModelEmployee->data(sqlModelEmployee->index(row, 0)).toString();
        QString nom = sqlModelEmployee->data(sqlModelEmployee->index(row, 1)).toString();
        QString prenom = sqlModelEmployee->data(sqlModelEmployee->index(row, 2)).toString();
        QString email = sqlModelEmployee->data(sqlModelEmployee->index(row, 3)).toString();
        QString telephone = sqlModelEmployee->data(sqlModelEmployee->index(row, 4)).toString();
        QString dateN = sqlModelEmployee->data(sqlModelEmployee->index(row, 5)).toString();
        QString role = sqlModelEmployee->data(sqlModelEmployee->index(row, 6)).toString();
        QString password = sqlModelEmployee->data(sqlModelEmployee->index(row, 8)).toString();

        // Check if any field contains the search text (case-insensitive)
        if (id.contains(searchLower, Qt::CaseInsensitive) ||
            nom.contains(searchLower, Qt::CaseInsensitive) ||
            prenom.contains(searchLower, Qt::CaseInsensitive) ||
            email.contains(searchLower, Qt::CaseInsensitive) ||
            telephone.contains(searchLower, Qt::CaseInsensitive) ||
            dateN.contains(searchLower, Qt::CaseInsensitive) ||
            role.contains(searchLower, Qt::CaseInsensitive) ||
            password.contains(searchLower, Qt::CaseInsensitive)) {
            // Add matching row to the model
            modelEmployee->setRowCount(rowCount + 1); // Increment row count
            modelEmployee->setData(modelEmployee->index(rowCount, 0), id.toInt());
            modelEmployee->setData(modelEmployee->index(rowCount, 1), nom);
            modelEmployee->setData(modelEmployee->index(rowCount, 2), prenom);
            modelEmployee->setData(modelEmployee->index(rowCount, 3), email);
            modelEmployee->setData(modelEmployee->index(rowCount, 4), telephone.toInt());
            modelEmployee->setData(modelEmployee->index(rowCount, 5), dateN);
            modelEmployee->setData(modelEmployee->index(rowCount, 6), role);
            modelEmployee->setData(modelEmployee->index(rowCount, 7), password);
            modelEmployee->setData(modelEmployee->index(rowCount, 8), "[Delete]");
            modelEmployee->setData(modelEmployee->index(rowCount, 9), "[Modify]");
            rowCount++;
        }
    }

    // Set the filtered model to the table
    ui->tableEmploye->setModel(modelEmployee);
    ui->tableEmploye->resizeColumnsToContents();
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

