#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QFileDialog>
#include <QStandardItemModel>

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
        // Skip photo
        // Password
        QString password = modelEmployee->data(modelEmployee->index(row, 7)).toString();

        // Switch to the modify form (index 2)
        ui->employeesNavBar->setCurrentIndex(2);

        // Populate the modify form fields (adjust these to match your UI)
        ui->cinInputM->setText(QString::number(idEmployee));
        ui->nameInputM->setText(nom);
        ui->prenomInputM->setText(prenom);
        ui->emailInputM->setText(email);
        ui->telephoneInputM->setText(QString::number(telephone));
        // Date input
        ui->dateNInputM->setDate(QDate::fromString(dateN, "dd/MM/yyyy"));

        ui->roleInputM->setCurrentText(role);
        // Skip photo
        ui->mdpInputM->setText(password);
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


void MainWindow::on_ajouterEmpBD_clicked()
{
    //Get the form variables
    int id_employee = ui->cinInput->text().toInt();
    std::string nomEmp = ui->nameInput->text().toStdString();
    std::string prenomEmp = ui->prenomInput->text().toStdString();
    std::string emailEmp = ui->emailInput->text().toStdString();
    int telephoneEmp = ui->telephoneInput->text().toInt();
    //Get date to insert it later into the oracle db
    std::string dateN = ui->dateNInput->text().toStdString();
    //Get combobox value as string
    std::string roleEmp = ui->roleInput->currentText().toStdString();
    std::vector<unsigned char> photo;

    //Get the password
    std::string password = ui->mdpInput->text().toStdString();
    //Create an employee object
    Employe e(id_employee, nomEmp, prenomEmp, emailEmp, telephoneEmp, dateN, roleEmp, photo, password);
    //Add the employee to the database
    bool test = e.ajouter();
    if(test)
    {
        ui->MessageForme->setText("Employe ajouté avec succès ✅");
        QMessageBox::information(nullptr, QObject::tr("Employee added"),
                                 QObject::tr("Employee added successfully.\n"
                                             "Click Cancel to exit."), QMessageBox::Cancel);
        refreshEmployeeTable();
    }
    else
    {
        ui->MessageForme->setText("Erreur :Employe non ajouté ❎");
        QMessageBox::critical(nullptr, QObject::tr("Employee not added"),
                              QObject::tr("Employee not added.\n"
                                          "Click Cancel to exit."), QMessageBox::Cancel);
    }
}


void MainWindow::on_photoInput_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "/home", tr("Image Files (*.png *.jpg *.bmp)"));
    ui->photoInput->setText(fileName);
}
