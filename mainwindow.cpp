#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "QMessageBox"

#include "etablissement.h"
#include "speechdialog.h"

#include <string>
#include <QString>

#include <QSqlTableModel>
#include <QTableView>
#include <QModelIndex>

#include <QDebug>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>

#include <QPdfWriter>
#include <QPainter>
#include <QFileDialog>
#include <QDateTime>
#include <QPainter>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLegend>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , engine(new QQmlApplicationEngine(this))
    , mapWindow(nullptr)
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

//Etablissement Navbar

void MainWindow::on_ajouterEtab_clicked()
{
    ui->etablissementsNavBar->setCurrentIndex(0);
}

void MainWindow::on_afficherEtab_clicked()
{
    ui->etablissementsNavBar->setCurrentIndex(1);
    Etablissement E;
    E.afficher(ui->tabV);
}

void MainWindow::on_modiferEtab_clicked()
{
    ui->etablissementsNavBar->setCurrentIndex(2);
}

// stat etab

void MainWindow::on_statsEtab_clicked()
{
    ui->etablissementsNavBar->setCurrentIndex(3);
    setupStatsChart();
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
    QString gouv = ui->combo->currentText();
    float longe = ui->long_2->text().toFloat();
    float lat = ui->lat->text().toFloat();
    int cap = ui->cap->text().toInt();
    QString mail = ui->mail->text();
    QString tele = ui->tel->text();
    int tel = tele.toInt();

    QRegularExpression regexNom("^[a-zA-ZÀ-ÖØ-öø-ÿ ]+$");
    QRegularExpression regexTel("^[0-9]+$");
    bool nomValide = regexNom.match(nom).hasMatch();
    bool telValide = regexTel.match(tele).hasMatch() && tele.length()==8;
    bool longeValide = longe > 0 && longe >=8.00 && longe<=11.10;// Intervalle de longitude pour la Tunisie
    bool latValide = lat > 0 && lat>=32.80 && lat<=37.35;// Intervalle de latitude pour la Tunisie
    bool mailValide = mail.contains("@") && mail.contains(".");
    bool capValide = cap > 0;
    bool gouvValide = gouv.isEmpty();

    if (!nomValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("Le nom doit contenir uniquement des lettres et des espaces!"), QMessageBox::Ok);
        return ;
    }
    else if (gouvValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("Veuillez sélectionner un gouvernorat!"), QMessageBox::Ok);
        return;
    }
    else if (!longeValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("Veuillez saisir une valeur valide pour longitude!"), QMessageBox::Ok);
        return ;
    } else if (!latValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("Veuillez saisir une valeur valide pour latitude!"), QMessageBox::Ok);
        return ;
    } else if (!mailValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("L'email doit contenir '@' et '.'!"), QMessageBox::Ok);
        return ;
    }
    else if (!telValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("verifier Le numéro de téléphone!"), QMessageBox::Ok);
        return ;
    } else if (!capValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("La capacité doit être un nombre positif!"), QMessageBox::Ok);
        return ;
    }
    Etablissement E( nom.toStdString(), gouv.toStdString() , longe , lat , cap , mail.toStdString() , tel);
    bool test = E.ajouter();
    if(test)
    {
        QMessageBox::information(nullptr, QObject::tr("Valider"), QObject::tr("Ajout effectué avec succès!!"), QMessageBox::Cancel);
        E.afficher(ui->tabV);
        ui->nomEtabInput->clear();
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

// supprimer un etablissement

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

// modifier etablissement

void MainWindow::on_ajouterEmp_8_clicked()
{
    int id = ui->tabV->model()->data(ui->tabV->model()->index(0, 0)).toInt();

    qDebug() << "ID récupéré:" << id;

    if (id == 0) {
        QMessageBox::warning(this, "Erreur", "Aucun ID valide sélectionné.");
        return;
    }

    QString nom1  = (ui->nom->text());
    QString gouv2 = (ui->gov3->text());
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
        ui->nom->clear();
        ui->gov3->clear();
        ui->long_3->clear();
        ui->lat_2->clear();
        ui->cap_2->clear();
        ui->mail_2->clear();
        ui->tel_2->clear();
    }

    else {
        QMessageBox::warning(this, "Erreur", "Échec de la modification.");
    }
}

// charger les donnés de l'etablissment selon id dans le formulaire de modification

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
        ui->gov3->setText(query.value(1).toString());
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

// export pdf

void MainWindow::on_pdfEtab_clicked()
{
    Etablissement E;
    E.afficher(ui->tabV);

    QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter en PDF"),
                                                    QDir::homePath() + "/etablissements.pdf",
                                                    tr("PDF Files (*.pdf)"));
    if (fileName.isEmpty()) {
        return;
    }

    QPdfWriter pdfWriter(fileName);
    pdfWriter.setPageSize(QPageSize::A4);
    pdfWriter.setResolution(300);

    QPainter painter(&pdfWriter);
    painter.setRenderHint(QPainter::Antialiasing);

    const int margin = 300;
    const int rowHeight = 250;
    const int colCountExpected = 8;
    const int pageWidth = pdfWriter.width() - 2 * margin;

    QVector<int> colWidths(colCountExpected);
    colWidths[0] = pageWidth * 0.05;  // ID
    colWidths[1] = pageWidth * 0.15;  // Nom
    colWidths[2] = pageWidth * 0.15;  // Gouvernorat
    colWidths[3] = pageWidth * 0.15;  // Longitude
    colWidths[4] = pageWidth * 0.15;  // Latitude
    colWidths[5] = pageWidth * 0.15;  // Capacité
    colWidths[6] = pageWidth * 0.15;  // Email
    colWidths[7] = pageWidth * 0.15;  // Téléphone

    const int columnSpacing = 30;
    int totalWidth = 0;
    for (int i = 0; i < colCountExpected; ++i) {
        totalWidth += colWidths[i];
    }
    totalWidth += columnSpacing * (colCountExpected - 1);
    if (totalWidth > pageWidth) {
        qDebug() << "Total width exceeds page width, adjusting proportionally.";
        float scaleFactor = static_cast<float>(pageWidth) / totalWidth;
        for (int i = 0; i < colCountExpected; ++i) {
            colWidths[i] = static_cast<int>(colWidths[i] * scaleFactor);
        }
    }

    const int fontSize = 8;
    const int headerFontSize = 10;
    const int maxRowsPerPage = 30;
    const int headerSpacing = 300;

    QFont headerFont("Arial", headerFontSize, QFont::Bold);
    QFont bodyFont("Arial", fontSize);
    painter.setPen(Qt::black);

    QSqlQueryModel* model = qobject_cast<QSqlQueryModel*>(ui->tabV->model());
    if (!model) {
        QMessageBox::warning(this, tr("Erreur"), tr("Impossible de récupérer les données."));
        return;
    }

    int rowCount = model->rowCount();
    int colCount = model->columnCount();
    if (colCount != colCountExpected) {
        qDebug() << "Nombre de colonnes inattendu :" << colCount;
    }

    QMap<QString, int> columnMap;
    columnMap["ID"] = -1;
    columnMap["Nom"] = -1;
    columnMap["Gouvernorat"] = -1;
    columnMap["Longitude"] = -1;
    columnMap["Latitude"] = -1;
    columnMap["Capacité"] = -1;
    columnMap["Email"] = -1;
    columnMap["Téléphone"] = -1;

    for (int col = 0; col < colCount; ++col) {
        QString header = model->headerData(col, Qt::Horizontal).toString().toLower();
        if (header.contains("id")) columnMap["ID"] = col;
        else if (header.contains("nom")) columnMap["Nom"] = col;
        else if (header.contains("gouvernorat")) columnMap["Gouvernorat"] = col;
        else if (header.contains("longitude")) columnMap["Longitude"] = col;
        else if (header.contains("latitude")) columnMap["Latitude"] = col;
        else if (header.contains("capacité")) columnMap["Capacité"] = col;
        else if (header.contains("email")) columnMap["Email"] = col;
        else if (header.contains("téléphone")) columnMap["Téléphone"] = col;
    }

    painter.setFont(QFont("Arial", 14, QFont::Bold));
    painter.drawText(margin, margin, tr("Liste des Établissements - %1")
                                         .arg(QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm")));
    int yPos = margin + 600;

    painter.setFont(headerFont);
    QStringList headers = {"ID", "Nom", "Gouvernorat", "Longitude", "Latitude",
                           "Capacité", "Email", "Téléphone"};
    int xPos = margin;
    for (int col = 0; col < colCountExpected; ++col) {
        QRect boundingRect = QRect(xPos, yPos, colWidths[col], rowHeight);
        painter.drawText(boundingRect, Qt::AlignLeft | Qt::TextWordWrap, headers[col]);
        xPos += colWidths[col] + columnSpacing;
    }

    yPos += rowHeight;
    painter.drawLine(margin, yPos, margin + pageWidth, yPos);
    yPos += headerSpacing;

    painter.setFont(bodyFont);
    int currentRowOnPage = 0;
    for (int row = 0; row < rowCount; ++row) {
        if (currentRowOnPage >= maxRowsPerPage || (yPos + rowHeight) > pdfWriter.height() - margin) {
            pdfWriter.newPage();
            yPos = margin;
            painter.setFont(headerFont);
            xPos = margin;
            for (int col = 0; col < colCountExpected; ++col) {
                QRect boundingRect = QRect(xPos, yPos, colWidths[col], rowHeight);
                painter.drawText(boundingRect, Qt::AlignLeft | Qt::TextWordWrap, headers[col]);
                xPos += colWidths[col] + columnSpacing;
            }
            yPos += rowHeight;
            painter.drawLine(margin, yPos, margin + pageWidth, yPos);
            yPos += headerSpacing;
            painter.setFont(bodyFont);
            currentRowOnPage = 0;
        }

        xPos = margin;
        for (int col = 0; col < colCountExpected; ++col) {
            QString header = headers[col];
            int modelCol = columnMap[header];
            QString text = (modelCol != -1) ? model->data(model->index(row, modelCol)).toString() : "N/A";
            QRect boundingRect = QRect(xPos, yPos, colWidths[col], rowHeight);
            painter.drawText(boundingRect, Qt::AlignLeft | Qt::TextWordWrap, text);
            xPos += colWidths[col] + columnSpacing;
        }
        yPos += rowHeight;
        currentRowOnPage++;
    }

    painter.end();

    QMessageBox::information(this, tr("Succès"), tr("Le fichier PDF a été généré avec succès."));
}

// rechercher un etablissment

void MainWindow::on_champRecherche_3_textChanged(const QString &arg1)
{
    QSqlQueryModel* model = new QSqlQueryModel();

    if (arg1.isEmpty()) {
        model->setQuery("SELECT ID_ETAB, NOM, GOUVERNORAT, LONGE, LAT, CAPACITE, MAIL, TEL FROM ETABLISSEMENTS");
    }
    else {

        QSqlQuery query;
        query.prepare("SELECT ID_ETAB, NOM, GOUVERNORAT, LONGE, LAT, CAPACITE, MAIL, TEL "
                      "FROM ETABLISSEMENTS WHERE NOM LIKE :nom");
        query.bindValue(":nom", "%" + arg1 + "%");

        if (!query.exec()) {
            qDebug() << "Erreur lors de l'exécution de la requête :" << query.lastError().text();
            delete model;
            return;
        }
        else
        {
            model->setQuery(query);
        }

        if (model->rowCount() == 0) {
            qDebug() << "Aucun résultat trouvé pour la recherche :" << arg1;
        }
    }

    if (model->lastError().isValid()) {
        qDebug() << "Erreur dans le modèle :" << model->lastError().text();
        delete model;
        return;
    }

    model->setHeaderData(0, Qt::Horizontal, QString("ID"));
    model->setHeaderData(1, Qt::Horizontal, QString("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QString("Gouvernorat"));
    model->setHeaderData(3, Qt::Horizontal, QString("Longitude"));
    model->setHeaderData(4, Qt::Horizontal, QString("Latitude"));
    model->setHeaderData(5, Qt::Horizontal, QString("Capacité"));
    model->setHeaderData(6, Qt::Horizontal, QString("Email"));
    model->setHeaderData(7, Qt::Horizontal, QString("Téléphone"));


    // Recuperer l'ancien modèle

    QAbstractItemModel* oldModel = ui->tabV->model();
    ui->tabV->setModel(model);
    if (oldModel) {
        delete oldModel;
    }

    QHeaderView* header = ui->tabV->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
}



// tri etablissement


void MainWindow::on_comboBox_3_activated(int index)
{
    qDebug() << "Slot on_comboBox_3_activated déclenché avec index :" << index;

    if (index == 0) {

        // Create a new QSqlQueryModel to hold the sorted data

        QSqlQueryModel* model = new QSqlQueryModel();

        // select all columns and sort by GOUVERNORAT

        QString queryString = "SELECT ID_ETAB, NOM, GOUVERNORAT, LONGE, LAT, CAPACITE, MAIL, TEL "
                              "FROM ETABLISSEMENTS ORDER BY GOUVERNORAT ASC";

        // Set the query to the model

        model->setQuery(queryString);

        if (model->lastError().isValid()) {
            qDebug() << "Erreur lors de l'exécution de la requête de tri :" << model->lastError().text();
            delete model;
            return;
        }

        // Set the headers for the table view

        model->setHeaderData(0, Qt::Horizontal, QString("ID"));
        model->setHeaderData(1, Qt::Horizontal, QString("Nom"));
        model->setHeaderData(2, Qt::Horizontal, QString("Gouvernorat"));
        model->setHeaderData(3, Qt::Horizontal, QString("Longitude"));
        model->setHeaderData(4, Qt::Horizontal, QString("Latitude"));
        model->setHeaderData(5, Qt::Horizontal, QString("Capacité"));
        model->setHeaderData(6, Qt::Horizontal, QString("Email"));
        model->setHeaderData(7, Qt::Horizontal, QString("Téléphone"));

        // Retrieve the old model from the table view

        QAbstractItemModel* oldModel = ui->tabV->model();

        // Set the new sorted model to the table view

        ui->tabV->setModel(model);


        if (oldModel) {
            delete oldModel;
        }
        else
        {
            QHeaderView* header = ui->tabV->horizontalHeader();
            header->setSectionResizeMode(QHeaderView::ResizeToContents);
        }
    }
}

// statistique etablissement

void MainWindow::setupStatsChart()
{
    // Récupérer les statistiques par gouvernorat

    QMap<QString, int> stats;

    QSqlQuery query;
    query.prepare("SELECT GOUVERNORAT, COUNT(*) as count FROM ETABLISSEMENTS GROUP BY GOUVERNORAT");

    if (query.exec()) {
        while (query.next()) {
            stats.insert(query.value(0).toString(), query.value(1).toInt());
        }
    } else {
        qDebug() << "Erreur lors de la récupération des statistiques:" << query.lastError().text();
        return;
    }

    // Créer un graphique

    QChart *chart = new QChart();
    chart->setTitle("Statistiques des établissements par gouvernorat");
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->setTheme(QChart::ChartThemeLight); // Style clair

    // Créer une série de données (courbe)

    QLineSeries *series = new QLineSeries();

    int index = 0;
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        series->append(index, it.value());
        index++;
    }

    chart->addSeries(series);

    // un diagramme en barres pour une meilleure visualisation

    QBarSeries *barSeries = new QBarSeries();
    QBarSet *set = new QBarSet("Nombre d'établissements");

    for (auto it = stats.begin(); it != stats.end(); ++it) {
        *set << it.value();
    }

    barSeries->append(set);

    chart->addSeries(barSeries);

    // Configurer les axes

    QStringList categories;

    for (auto it = stats.begin(); it != stats.end(); ++it) {
        categories << it.key();
    }

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    barSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat("%d");
    axisY->setTitleText("Nombre d'établissements");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    barSeries->attachAxis(axisY);

    // Ajouter une légende

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // Personnalisation des couleurs

    series->setColor(QColor(65, 105, 225)); // Bleu royal
    set->setColor(QColor(100, 149, 237)); // Bleu ciel

    // Personnalisation du fond

    chart->setBackgroundBrush(QBrush(QColor(240, 240, 240)));
    chart->setBackgroundRoundness(10);

    // Personnalisation des polices

    QFont font;
    font.setPixelSize(12);
    chart->setTitleFont(font);
    axisX->setLabelsFont(font);
    axisY->setLabelsFont(font);

    // Créer la vue du graphique

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // Supprimer l'ancien widget s'il existe

    if (ui->frame_10->layout()) {
        QLayoutItem* item;
        while ((item = ui->frame_10->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete ui->frame_10->layout();
    }

    // zone au le stat sera affichié

    QVBoxLayout *layout = new QVBoxLayout(ui->frame_10);
    layout->addWidget(chartView);
    ui->frame_10->setLayout(layout);
}


// geolocalisation

void MainWindow::on_geoBTN_clicked()
{
    if (mapWindow) {

        mapWindow->setProperty("visible", true);

        return;
    }

    // Charger file Mapview.qml les ressources

    const QUrl url(QStringLiteral("qrc:/Mapview.qml"));

    engine->addImportPath("qrc:/res"); // Ajout du chemin d'import

    qDebug() << "Tentative de chargement de l'URL :" << url.toString();

    engine->load(url);

    // Vérifier les erreurs

    if (engine->rootObjects().isEmpty()) {

        qDebug() << "Erreur : Impossible de charger Mapview.qml";

        return;
    }

    mapWindow = engine->rootObjects().first();

    if (!mapWindow) {

        qDebug() << "Erreur : Aucune fenêtre QML n'a été trouvée";
        return;
    }

    QObject::connect(mapWindow, SIGNAL(destroyed()), this, SLOT(onMapWindowClosed()));
}

void MainWindow::onMapWindowClosed()
{
    mapWindow = nullptr;
}
