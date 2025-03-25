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
    , cap(nullptr) // Initialize camera pointer
    , timer(nullptr) // Initialize timer

{
    ui->setupUi(this);
    ui->cameraLabel->hide();
    refreshEmployeeTable();
    connect(ui->tableEmploye, &QTableView::clicked, this, &MainWindow::onEmployeeTableClicked);
    refreshStats();

    // Initialize DNN models
    detector = cv::FaceDetectorYN::create("C:/models/face_detection_yunet_2023mar.onnx", "", cv::Size(320, 320));
    detector->setScoreThreshold(0.6); // Lowered for more detections
    detector->setNMSThreshold(0.3);
    detector->setTopK(5000);
    recognizer = cv::FaceRecognizerSF::create("C:/models/face_recognition_sface_2021dec.onnx", "");


    // Load known features
    loadKnownFeatures();

    qDebug() << "Qt Version:" << QT_VERSION_STR;
    /*QFile file("C:\\opencv-4.9.0minGW\\install\\etc\\haarcascades\\haarcascade_frontalface_default.xml");
    if (!file.exists()) {
        qDebug() << "Haar cascade file does not exist at specified path!";
    } else {
        qDebug() << "File found, attempting to load...";
    }*/
}

MainWindow::~MainWindow()
{
    if (cap && cap->isOpened()) {
        cap->release();
    }
    delete cap;
    delete timer;
    delete ui;
}

//FACIAL RECOGNITION
void MainWindow::loadKnownFeatures()
{
    knownFeatures.clear();
    QSqlQuery query("SELECT id_employe, photo FROM employe");
    while (query.next()) {
        int id = query.value(0).toInt();
        QByteArray photoData = query.value(1).toByteArray();

        if (!photoData.isEmpty()) {
            cv::Mat img = cv::imdecode(cv::Mat(1, photoData.size(), CV_8UC1, photoData.data()), cv::IMREAD_COLOR);
            if (!img.empty()) {
                // Resize to consistent size
                cv::resize(img, img, cv::Size(320, 320));
                detector->setInputSize(img.size());
                cv::Mat faces;
                detector->detect(img, faces);
                if (faces.rows == 1) { // Only one face per photo
                    cv::Mat alignedFace;
                    recognizer->alignCrop(img, faces.row(0), alignedFace);
                    if (!alignedFace.empty()) {
                        // Save for debugging
                        QString debugFile = QString("db_face_id_%1.jpg").arg(id);
                        cv::imwrite(debugFile.toStdString(), alignedFace);
                        qDebug() << "Saved DB aligned face for ID" << id << "to" << debugFile;

                        cv::Mat feature;
                        recognizer->feature(alignedFace, feature);
                        knownFeatures.push_back({id, feature.clone()}); // Single feature per ID
                        qDebug() << "Loaded feature for ID:" << id;
                    } else {
                        qDebug() << "Alignment failed for ID:" << id;
                    }
                } else {
                    qDebug() << "Invalid face count for ID" << id << ":" << faces.rows << "faces detected";
                }
            } else {
                qDebug() << "Failed to decode photo for ID:" << id;
            }
        } else {
            qDebug() << "Empty photo data for ID:" << id;
        }
    }
    qDebug() << "Loaded features for" << knownFeatures.size() << "employees.";

    // Debug distances between known features
    for (size_t i = 0; i < knownFeatures.size(); ++i) {
        for (size_t j = i + 1; j < knownFeatures.size(); ++j) {
            double distance = recognizer->match(knownFeatures[i].second, knownFeatures[j].second, cv::FaceRecognizerSF::FR_COSINE);
            qDebug() << "Distance between ID" << knownFeatures[i].first << "and ID" << knownFeatures[j].first << ":" << distance;
        }
    }
}



void MainWindow::processFrameAndUpdateGUI()
{
    if (!cap || !cap->isOpened()) {
        qDebug() << "Camera not initialized.";
        return;
    }

    cv::Mat frame;
    *cap >> frame;
    if (frame.empty()) {
        qDebug() << "No frame captured.";
        return;
    }

    // Resize frame for consistency
    cv::Mat resizedFrame;
    cv::resize(frame, resizedFrame, cv::Size(320, 320));
    detector->setInputSize(resizedFrame.size());

    // Detect faces
    cv::Mat faces;
    detector->detect(resizedFrame, faces);
    qDebug() << "Detected" << faces.rows << "faces";

    cv::Mat displayFrame = resizedFrame.clone();
    const double threshold = 0.4; // Stricter threshold

    if (faces.rows > 0) {
        // Process the first face
        cv::Rect bbox(faces.at<float>(0, 0), faces.at<float>(0, 1), faces.at<float>(0, 2), faces.at<float>(0, 3));
        cv::rectangle(displayFrame, bbox, cv::Scalar(0, 255, 0), 2);

        // Align and extract feature
        cv::Mat alignedFace;
        recognizer->alignCrop(resizedFrame, faces.row(0), alignedFace);
        if (alignedFace.empty()) {
            qDebug() << "Alignment failed for live frame.";
            cv::putText(displayFrame, "Alignment Failed", cv::Point(10, 30),
                        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
        } else {
            // Save live aligned face for debugging with timestamp
            QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
            QString debugFile = QString("live_face_%1.jpg").arg(timestamp);
            cv::imwrite(debugFile.toStdString(), alignedFace);
            qDebug() << "Saved live aligned face to" << debugFile;

            cv::Mat feature;
            recognizer->feature(alignedFace, feature);

            // Find best match with separation check
            double minDistance = DBL_MAX, secondMinDistance = DBL_MAX;
            int bestId = -1;
            for (const auto& known : knownFeatures) {
                double distance = recognizer->match(feature, known.second, cv::FaceRecognizerSF::FR_COSINE);
                qDebug() << "Distance to ID" << known.first << ":" << distance;
                if (distance < minDistance) {
                    secondMinDistance = minDistance;
                    minDistance = distance;
                    bestId = known.first;
                } else if (distance < secondMinDistance) {
                    secondMinDistance = distance;
                }
            }

            if (minDistance < threshold && (secondMinDistance - minDistance > 0.1)) { // Clear separation
                QString text = QString("ID: %1 (Dist: %2)").arg(bestId).arg(minDistance, 0, 'f', 2);
                cv::putText(displayFrame, text.toStdString(), cv::Point(bbox.x, bbox.y - 10),
                            cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);
                qDebug() << "Matched ID:" << bestId << "with distance:" << minDistance;
            } else {
                cv::putText(displayFrame, "Unknown", cv::Point(bbox.x, bbox.y - 10),
                            cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
                qDebug() << "No clear match. Best:" << minDistance << "Second:" << secondMinDistance;
            }
        }
    } else {
        cv::putText(displayFrame, "No Face Detected", cv::Point(10, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
    }

    // Display frame
    cv::cvtColor(displayFrame, displayFrame, cv::COLOR_BGR2RGB);
    QImage qimg(displayFrame.data, displayFrame.cols, displayFrame.rows, displayFrame.step, QImage::Format_RGB888);
    ui->cameraLabel->setPixmap(QPixmap::fromImage(qimg).scaled(ui->cameraLabel->size(), Qt::KeepAspectRatio));
}
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
    refreshStats(); // Update stats after refreshing the table
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
                refreshStats(); // Refresh stats after deleting
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
        newPhotoSelected = false;
        currentPhotoPath = "";

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
    QString dateNRaw = model->data(model->index(row, 5)).toString(); // Raw date from table
    QString role = model->data(model->index(row, 6)).toString();
    QString password = model->data(model->index(row, 7)).toString();

    // Parse the date and strip time
    QString dateN;
    if (dateNRaw.contains("T")) { // Check for ISO 8601 format like "2000-01-01T00:00:00.000"
        QDateTime dateTime = QDateTime::fromString(dateNRaw, Qt::ISODate); // Parse ISO 8601
        dateN = dateTime.isValid() ? dateTime.date().toString("MM/dd/yyyy") : dateNRaw;
    } else if (dateNRaw.contains("-") && !dateNRaw.contains(":")) { // Check for "DD-MON-RR" or "YYYY-MM-DD"
        QDate date = QDate::fromString(dateNRaw, "yyyy-MM-dd"); // Try "YYYY-MM-DD"
        if (!date.isValid()) {
            date = QDate::fromString(dateNRaw, "dd-MMM-yy"); // Fallback to "DD-MON-RR"
            if (date.year() < 1970 && date.isValid()) {
                date = date.addYears(100); // Adjust for 20xx century
            }
        }
        dateN = date.isValid() ? date.toString("MM/dd/yyyy") : dateNRaw;
    } else {
        dateN = dateNRaw; // Fallback to raw if unrecognized
    }
    qDebug() << "Raw date from table:" << dateNRaw << "Formatted date:" << dateN;

    // Fetch photo
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
    pdfWriter.setResolution(300); // 300 DPI

    QPainter painter(&pdfWriter);
    painter.setRenderHint(QPainter::Antialiasing);

    // Page dimensions
    const int pageWidth = pdfWriter.width();  // ~2480 pixels
    const int pageHeight = pdfWriter.height(); // ~3508 pixels
    const int margin = 500;  // ~1.67 cm
    const int lineSpacing = 250; // ~0.83 cm
    const int maxPhotoWidth = 450;  // ~1.5 cm
    const int maxPhotoHeight = 450; // ~1.5 cm

    // Header background (light blue)
    painter.setBrush(QBrush(QColor(200, 220, 255)));
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, 0, pageWidth, 800);

    // Header title
    QFont titleFont("Arial", 18, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(Qt::black);
    painter.drawText(margin, 400, "Employee Profile");

    // Photo (top-right corner, no margin)
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

    // Employee details section
    int yPos = 800; // Start below header
    QFont labelFont("Arial", 12, QFont::Bold);
    QFont valueFont("Arial", 12);
    const int labelX = margin + 50;      // ~550 pixels
    const int valueX = margin + 600;     // ~950 pixels
    const int maxValueWidth = pageWidth - valueX - margin; // ~1030 pixels

    // Draw a box around details
    painter.setPen(QPen(Qt::gray, 5));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(margin, yPos - lineSpacing / 2, pageWidth - 2 * margin, 8 * lineSpacing + 50);

    // Details with labels
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
    QString dateNText = dateN; // Formatted as MM/DD/YYYY
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

    // Footer
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
    if (cap && cap->isOpened()) {
        timer->stop();
        cap->release();
        delete cap;
        cap = nullptr;
        ui->cameraLabel->hide();
        qDebug() << "Camera stopped.";
        return;
    }

    cap = new cv::VideoCapture(0);
    if (!cap->isOpened()) {
        QMessageBox::critical(this, "Error", "Could not open camera.");
        delete cap;
        cap = nullptr;
        return;
    }

    // Match the resolution used for database photos
    cap->set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap->set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    qDebug() << "Camera resolution set to:" << cap->get(cv::CAP_PROP_FRAME_WIDTH) << "x" << cap->get(cv::CAP_PROP_FRAME_HEIGHT);

    if (knownFeatures.empty()) {
        QMessageBox::warning(this, "Error", "No employee features found in database.");
        cap->release();
        delete cap;
        cap = nullptr;
        return;
    }

    ui->cameraLabel->show();
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::processFrameAndUpdateGUI);
    timer->start(33);
}
