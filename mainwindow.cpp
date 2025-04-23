#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "equipements.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QSqlError>
#include <QTableWidgetItem>
#include <QPixmap>
#include <QDebug>
#include <QTextDocument>
#include <QtPrintSupport/QPrinter>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtGlobal>
#include "statistics_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), statsWindow(nullptr) {
    ui->setupUi(this);

    imagePathAjout = "";
    imagePathModif = "";
    currentModificationId = -1;

    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onClarifaiReplyFinished);

    if (!ui->imagePreviewLabel) qDebug() << "imagePreviewLabel est manquant";
    if (!ui->imagePreviewLabel_3) qDebug() << "imagePreviewLabel_3 est manquant";
    if (!ui->tableWidgetEq) qDebug() << "tableWidgetEq est manquant";

    ui->dateEqinput->setDisplayFormat("dd/MM/yyyy");
    ui->dateEqinput->setDate(QDate::currentDate());
    ui->dateEqinput_3->setDisplayFormat("dd/MM/yyyy");
    ui->dateEqinput_3->setDate(QDate::currentDate());

    ui->etatEqinput->addItems({"Neuf", "Utilisé", "En panne"});
    ui->etatEqinput_3->addItems({"Neuf", "Utilisé", "En panne"});

    ui->tableWidgetEq->setColumnCount(8);
    ui->tableWidgetEq->setHorizontalHeaderLabels(
        QStringList() << "ID" << "Nom" << "Type" << "État" << "Marque" << "Quantité" << "Date" << "Photo");
    ui->tableWidgetEq->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidgetEq->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetEq->setIconSize(QSize(100, 100));
}

MainWindow::~MainWindow() {
    delete statsWindow;
    delete ui;
}

void MainWindow::on_statsEq_clicked() {
    qDebug() << "on_statsEq_clicked called";
    qDebug() << "Number of tabs in equipementsNavBar:" << ui->equipementsNavBar->count();
    if (ui->equipementsNavBar->count() <= 3) {
        qDebug() << "Tab index 3 does not exist";
        QMessageBox::warning(this, "Erreur", "L'onglet Statistiques n'existe pas (index 3).");
        return;
    }

    ui->equipementsNavBar->setCurrentIndex(3);
    qDebug() << "Set tab index to 3";

    if (!statsWindow) {
        qDebug() << "Creating new StatisticsWindow";
        statsWindow = new StatisticsWindow(this);
    }
    qDebug() << "Calling updatePieChart";
    statsWindow->updatePieChart();
    qDebug() << "Showing StatisticsWindow";
    statsWindow->show();
    statsWindow->raise();
}

void MainWindow::on_AjouterEquipement_clicked() {
    ui->AjouterEquipement->setEnabled(false);

    QString idQstr = ui->idEqinput->text();
    QString nomQStr = ui->nomEqinput->text();
    QString typeQStr = ui->typeEqInput->text();
    QString etatQStr = ui->etatEqinput->currentText();
    QString marqueQStr = ui->marqueEqinput->text();
    QString quantiteQStr = ui->quantiteEqinput->text();
    QDate date = ui->dateEqinput->date();

    if (idQstr.isEmpty() || nomQStr.isEmpty() || typeQStr.isEmpty() || etatQStr.isEmpty() ||
        marqueQStr.isEmpty() || quantiteQStr.isEmpty() || !date.isValid()) {
        QMessageBox::warning(this, "Erreur", "Tous les champs doivent être remplis.");
        ui->AjouterEquipement->setEnabled(true);
        return;
    }

    bool okId, okQt;
    int id = idQstr.toInt(&okId);
    int quantite = quantiteQStr.toInt(&okQt);

    if (!okId || id <= 0) {
        QMessageBox::warning(this, "Erreur", "L'ID doit être un nombre valide positif.");
        ui->AjouterEquipement->setEnabled(true);
        return;
    }
    if (!okQt || quantite <= 0) {
        QMessageBox::warning(this, "Erreur", "La quantité doit être un nombre valide positif.");
        ui->AjouterEquipement->setEnabled(true);
        return;
    }
    if (date > QDate::currentDate()) {
        QMessageBox::warning(this, "Erreur", "La date ne peut pas être dans le futur.");
        ui->AjouterEquipement->setEnabled(true);
        return;
    }

    if (imagePathAjout.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner une image.");
        ui->AjouterEquipement->setEnabled(true);
        return;
    }

    QFile file(imagePathAjout);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Erreur", "Impossible de charger l'image.");
        ui->AjouterEquipement->setEnabled(true);
        return;
    }

    QByteArray imageData = file.readAll();
    file.close();

    if (imageData.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "L'image est vide ou corrompue.");
        ui->AjouterEquipement->setEnabled(true);
        return;
    }

    QNetworkRequest request(QUrl("https://api.clarifai.com/v2/models/aaa03c23b3724a16a56b629203edc62c/outputs"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Key cd1da61feecc4fc4a08514fb2150130c");

    QString base64Image = imageData.toBase64();
    qDebug() << "Base64 Image (premier 100 caractères) :" << base64Image.left(100) << "...";

    QJsonObject mainObj;
    QJsonArray inputsArray;
    QJsonObject inputObj;
    QJsonObject dataObj;
    QJsonObject imageObj;

    imageObj["base64"] = base64Image;
    dataObj["image"] = imageObj;
    inputObj["data"] = dataObj;
    inputsArray.append(inputObj);
    mainObj["inputs"] = inputsArray;

    QJsonDocument doc(mainObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    qDebug() << "Requête JSON envoyée à Clarifai :" << jsonData;

    networkManager->post(request, jsonData);

    ui->AjouterEquipement->setProperty("id", id);
    ui->AjouterEquipement->setProperty("nom", nomQStr);
    ui->AjouterEquipement->setProperty("type", typeQStr);
    ui->AjouterEquipement->setProperty("etat", etatQStr);
    ui->AjouterEquipement->setProperty("marque", marqueQStr);
    ui->AjouterEquipement->setProperty("quantite", quantite);
    ui->AjouterEquipement->setProperty("dateSqlite", date.toString("yyyy-MM-dd"));
    ui->AjouterEquipement->setProperty("imageData", imageData);
}

void MainWindow::on_afficherEq_clicked() {
    ui->equipementsNavBar->setCurrentIndex(1);
    QSqlQuery query = Equipements::afficherEq();

    if (!query.isActive()) {
        QMessageBox::critical(this, "Erreur", "Impossible de récupérer les équipements : " + query.lastError().text());
        return;
    }

    ui->tableWidgetEq->setRowCount(0);
    int row = 0;

    while (query.next()) {
        ui->tableWidgetEq->insertRow(row);
        for (int col = 0; col < 8; col++) {
            if (col == 7) {
                QByteArray imageData = query.value("IMAGE_EQ").toByteArray();
                if (!imageData.isEmpty()) {
                    QPixmap pixmap;
                    if (pixmap.loadFromData(imageData)) {
                        QTableWidgetItem *imageItem = new QTableWidgetItem();
                        imageItem->setData(Qt::DecorationRole, pixmap.scaled(100, 100, Qt::KeepAspectRatio));
                        ui->tableWidgetEq->setItem(row, col, imageItem);
                    } else {
                        ui->tableWidgetEq->setItem(row, col, new QTableWidgetItem("Image corrompue"));
                    }
                } else {
                    ui->tableWidgetEq->setItem(row, col, new QTableWidgetItem("Pas d'image"));
                }
            } else {
                ui->tableWidgetEq->setItem(row, col, new QTableWidgetItem(query.value(col).toString()));
            }
        }
        row++;
    }
}

void MainWindow::on_supprimerEq_clicked() {
    int row = ui->tableWidgetEq->currentRow();
    if (row == -1) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un équipement à supprimer.");
        return;
    }

    int id = ui->tableWidgetEq->item(row, 0)->text().toInt();
    Equipements eq;
    if (eq.supprimerEq(id)) {
        QMessageBox::information(this, "Succès", "Équipement supprimé avec succès !");
        ui->tableWidgetEq->removeRow(row);
    } else {
        QMessageBox::warning(this, "Erreur", "Échec de la suppression de l'équipement.");
    }
}

void MainWindow::on_confirmerModification_clicked() {
    if (currentModificationId == -1) {
        QMessageBox::warning(this, "Erreur", "Aucun équipement sélectionné pour modification.");
        return;
    }

    QString nomQStr = ui->nomEqinput_3->text();
    QString typeQStr = ui->typeEqInput_3->text();
    QString etatQStr = ui->etatEqinput_3->currentText();
    QString marqueQStr = ui->marqueEqinput_3->text();
    QString quantiteQStr = ui->quantiteEqinput_3->text();
    QDate date = ui->dateEqinput_3->date();

    if (nomQStr.isEmpty() || typeQStr.isEmpty() || etatQStr.isEmpty() ||
        marqueQStr.isEmpty() || quantiteQStr.isEmpty() || !date.isValid()) {
        QMessageBox::warning(this, "Erreur", "Tous les champs doivent être remplis.");
        return;
    }

    bool okQt;
    int quantite = quantiteQStr.toInt(&okQt);
    if (!okQt || quantite <= 0) {
        QMessageBox::warning(this, "Erreur", "La quantité doit être un nombre valide positif.");
        return;
    }
    if (date > QDate::currentDate()) {
        QMessageBox::warning(this, "Erreur", "La date ne peut pas être dans le futur.");
        return;
    }

    QString dateSqlite = date.toString("yyyy-MM-dd");
    std::string nom = nomQStr.toStdString();
    std::string type = typeQStr.toStdString();
    std::string etat = etatQStr.toStdString();
    std::string marque = marqueQStr.toStdString();

    std::vector<unsigned char> photo;
    if (!imagePathModif.isEmpty()) {
        QFile file(imagePathModif);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray newImageData = file.readAll();
            photo = std::vector<unsigned char>(newImageData.begin(), newImageData.end());
            file.close();
        } else {
            QMessageBox::warning(this, "Erreur", "Impossible de charger la nouvelle image.");
            return;
        }
    } else {
        QSqlQuery query;
        query.prepare("SELECT IMAGE_EQ FROM EQUIPEMENTS WHERE ID_EQ = :id");
        query.bindValue(":id", currentModificationId);
        if (query.exec() && query.next()) {
            QByteArray imageData = query.value("IMAGE_EQ").toByteArray();
            photo = std::vector<unsigned char>(imageData.begin(), imageData.end());
        }
    }

    Equipements eq(currentModificationId, nom, etat, type, quantite, photo, dateSqlite.toStdString(), marque);
    if (eq.modifierEq(currentModificationId)) {
        QMessageBox::information(this, "Succès", "Équipement modifié avec succès !");
        ui->equipementsNavBar->setCurrentIndex(1);
        on_afficherEq_clicked();
    } else {
        QMessageBox::warning(this, "Erreur", "Échec de la modification de l'équipement.");
    }
}

void MainWindow::on_modifierEq_clicked() {
    int row = ui->tableWidgetEq->currentRow();
    if (row == -1) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un équipement à modifier.");
        return;
    }

    currentModificationId = ui->tableWidgetEq->item(row, 0)->text().toInt();
    ui->equipementsNavBar->setCurrentIndex(2);

    QSqlQuery query;
    query.prepare("SELECT * FROM EQUIPEMENTS WHERE ID_EQ = :id");
    query.bindValue(":id", currentModificationId);

    if (!query.exec() || !query.next()) {
        QMessageBox::warning(this, "Erreur", "Impossible de récupérer les données de l'équipement : " + query.lastError().text());
        return;
    }

    ui->idEqinput_3->setText(QString::number(currentModificationId));
    ui->nomEqinput_3->setText(query.value("NOM_EQ").toString());
    ui->typeEqInput_3->setText(query.value("TYPEEQ").toString());
    ui->etatEqinput_3->setCurrentText(query.value("ETATEQ").toString());
    ui->marqueEqinput_3->setText(query.value("MARQUEEQ").toString());
    ui->quantiteEqinput_3->setText(query.value("QT").toString());
    ui->dateEqinput_3->setDate(QDate::fromString(query.value("DATEEQ").toString(), "yyyy-MM-dd"));

    QByteArray imageData = query.value("IMAGE_EQ").toByteArray();
    if (!imageData.isEmpty() && ui->imagePreviewLabel_3) {
        QPixmap pixmap;
        if (pixmap.loadFromData(imageData)) {
            ui->imagePreviewLabel_3->setPixmap(pixmap.scaled(300, 300, Qt::KeepAspectRatio));
        } else {
            ui->imagePreviewLabel_3->clear();
            QMessageBox::warning(this, "Erreur", "Impossible de charger l'image pour la modification.");
        }
    } else {
        ui->imagePreviewLabel_3->clear();
    }
    imagePathModif = "";
}

void MainWindow::on_selectImageButton_clicked() {
    QString path = QFileDialog::getOpenFileName(this, "Sélectionner une image",
                                                QString(), "Images (*.png *.jpg *.jpeg)");
    if (path.isEmpty()) return;

    QPixmap pixmap(path);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "Erreur", "Impossible de charger l'image sélectionnée.");
        return;
    }

    if (sender() == ui->selectImageButton) {
        imagePathAjout = path;
        if (ui->imagePreviewLabel) {
            ui->imagePreviewLabel->setPixmap(pixmap.scaled(300, 300, Qt::KeepAspectRatio));
        }
    } else if (sender() == ui->selectImageButton_3) {
        imagePathModif = path;
        if (ui->imagePreviewLabel_3) {
            ui->imagePreviewLabel_3->setPixmap(pixmap.scaled(300, 300, Qt::KeepAspectRatio));
        }
    }
}

void MainWindow::on_equipementsBTN_clicked() {
    ui->equipementsNavBar->setCurrentIndex(0);
}

void MainWindow::on_ajouterEq_clicked() {
    ui->equipementsNavBar->setCurrentIndex(0);
}

#include <QFile>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>

void MainWindow::on_pdfEmp_5_clicked() {
    QString html = "<html><head>"
                   "<style>"
                   "table { border-collapse: collapse; width: 100%; font-family: Arial, sans-serif; }"
                   "th, td { border: 1px solid black; padding: 8px; text-align: left; }"
                   "th { background-color: #0E3B52; color: white; }"
                   "tr:nth-child(even) { background-color: #f2f2f2; }"
                   "</style>"
                   "</head><body>"
                   "<h1 style='text-align: center;'>Liste des Équipements</h1>"
                   "<table>";

    html += "<tr>";
    for (int col = 0; col < ui->tableWidgetEq->columnCount() - 1; col++) {
        html += "<th>" + ui->tableWidgetEq->horizontalHeaderItem(col)->text() + "</th>";
    }
    html += "</tr>";

    for (int row = 0; row < ui->tableWidgetEq->rowCount(); row++) {
        html += "<tr>";
        for (int col = 0; col < ui->tableWidgetEq->columnCount() - 1; col++) {
            if (col == 7) {
                html += "<td>Image</td>";
            } else {
                QTableWidgetItem *item = ui->tableWidgetEq->item(row, col);
                QString text = item ? item->text() : "";
                html += "<td>" + text + "</td>";
            }
        }
        html += "</tr>";
    }

    html += "</table></body></html>";

    QString fileName = QFileDialog::getSaveFileName(this, "Sauvegarder en HTML",
                                                    QDir::homePath() + "/equipements.html", "HTML Files (*.html)");

    if (fileName.isEmpty()) {
        return;
    }

    if (!fileName.endsWith(".html", Qt::CaseInsensitive)) {
        fileName += ".html";
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Erreur", "Impossible de créer le fichier HTML !");
        return;
    }

    QTextStream out(&file);
    out << html;
    file.close();

    QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));

    QMessageBox::information(this, "Succès",
                             "Le fichier HTML a été généré avec succès !\n"
                             "Vous pouvez l'imprimer en PDF depuis votre navigateur.");
}

void MainWindow::on_comboBox_6_currentIndexChanged(int index) {
    if (index < 0 || ui->tableWidgetEq->rowCount() == 0) {
        return;
    }

    int columnToSort;
    switch (index) {
    case 0:
        columnToSort = 0;
        break;
    case 1:
        columnToSort = 1;
        break;
    case 2:
        columnToSort = 4;
        break;
    default:
        return;
    }

    Qt::SortOrder order = Qt::AscendingOrder;
    ui->tableWidgetEq->sortItems(columnToSort, order);

    QMessageBox::information(this, "Tri", "Tableau trié par " + ui->comboBox_6->currentText());
}

void MainWindow::on_champRecherche_6_textChanged(const QString &text) {
    QString searchText = text.trimmed().toLower();

    for (int row = 0; row < ui->tableWidgetEq->rowCount(); ++row) {
        bool match = false;
        for (int col = 0; col < ui->tableWidgetEq->columnCount() - 1; ++col) {
            QTableWidgetItem *item = ui->tableWidgetEq->item(row, col);
            if (item && item->text().toLower().contains(searchText)) {
                match = true;
                break;
            }
        }
        ui->tableWidgetEq->setRowHidden(row, !match);
    }
}

void MainWindow::onClarifaiReplyFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = reply->errorString();
        QString serverResponse = reply->readAll();
        qDebug() << "Erreur Clarifai :" << errorMsg;
        qDebug() << "Réponse du serveur :" << serverResponse;
        QMessageBox::warning(this, "Erreur", "Erreur lors de la vérification de l'image :\n" + errorMsg + "\nRéponse du serveur : " + serverResponse);
        ui->AjouterEquipement->setEnabled(true);
        reply->deleteLater();
        return;
    }

    QByteArray response = reply->readAll();
    qDebug() << "Réponse de Clarifai :" << response;

    QJsonDocument doc = QJsonDocument::fromJson(response);
    if (doc.isNull()) {
        QMessageBox::warning(this, "Erreur", "Réponse invalide de Clarifai (JSON incorrect).");
        ui->AjouterEquipement->setEnabled(true);
        reply->deleteLater();
        return;
    }

    QJsonObject json = doc.object();
    QJsonArray outputs = json["outputs"].toArray();
    if (outputs.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Aucune donnée renvoyée par Clarifai.");
        ui->AjouterEquipement->setEnabled(true);
        reply->deleteLater();
        return;
    }

    QJsonArray concepts = outputs[0].toObject()["data"].toObject()["concepts"].toArray();
    QString expectedName = ui->AjouterEquipement->property("nom").toString().toLower();
    bool matchFound = false;

    for (const QJsonValue &concept : concepts) {
        QString detectedName = concept.toObject()["name"].toString().toLower();
        double confidence = concept.toObject()["value"].toDouble();
        qDebug() << "Concept détecté :" << detectedName << " (confiance :" << confidence << ")";
        if (detectedName.contains(expectedName) && confidence > 0.9) {
            matchFound = true;
            break;
        }
    }

    if (!matchFound) {
        QMessageBox::warning(this, "Erreur", "L'image ne correspond pas au nom de l'équipement (" + expectedName + ").");
        ui->AjouterEquipement->setEnabled(true);
        reply->deleteLater();
        return;
    }

    int id = ui->AjouterEquipement->property("id").toInt();
    QString nomQStr = ui->AjouterEquipement->property("nom").toString();
    QString typeQStr = ui->AjouterEquipement->property("type").toString();
    QString etatQStr = ui->AjouterEquipement->property("etat").toString();
    QString marqueQStr = ui->AjouterEquipement->property("marque").toString();
    int quantite = ui->AjouterEquipement->property("quantite").toInt();
    QString dateSqlite = ui->AjouterEquipement->property("dateSqlite").toString();
    QByteArray imageData = ui->AjouterEquipement->property("imageData").toByteArray();

    std::string nom = nomQStr.toStdString();
    std::string type = typeQStr.toStdString();
    std::string etat = etatQStr.toStdString();
    std::string marque = marqueQStr.toStdString();
    std::vector<unsigned char> photo(imageData.begin(), imageData.end());

    Equipements eq(id, nom, etat, type, quantite, photo, dateSqlite.toStdString(), marque);
    if (eq.ajouterEq()) {
        QMessageBox::information(this, "Succès", "Équipement ajouté avec succès !");
        ui->idEqinput->clear();
        ui->nomEqinput->clear();
        ui->typeEqInput->clear();
        ui->etatEqinput->setCurrentIndex(0);
        ui->marqueEqinput->clear();
        ui->quantiteEqinput->clear();
        ui->dateEqinput->setDate(QDate::currentDate());
        imagePathAjout = "";
        if (ui->imagePreviewLabel) {
            ui->imagePreviewLabel->clear();
        }
        on_afficherEq_clicked();
    } else {
        QMessageBox::warning(this, "Erreur", "Échec de l'ajout de l'équipement.");
    }

    ui->AjouterEquipement->setEnabled(true);
    reply->deleteLater();
}

void MainWindow::on_pushButton_clicked() {
    QMessageBox::information(this, "Info", "Fonctionnalité non implémentée.");
}

void MainWindow::on_deconnexionBTN_clicked() {
    QApplication::quit();
}
