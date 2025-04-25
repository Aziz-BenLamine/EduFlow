#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "QMessageBox"

#include "etablissement.h"

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

#include <QTextToSpeech>
#include <QVoice>
#include <QThread>

#include <QVBoxLayout>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , engine(new QQmlApplicationEngine(this))
    , mapWindow(nullptr)
    , speech(new QTextToSpeech(this)) // Initialisation de QTextToSpeech
    , speechDialog(nullptr)           // Initialisation du dialogue
    , textInput(nullptr)

{
    ui->setupUi(this);

    // Configuration de QTextToSpeech
    speech->setVolume(0.7); // Volume à 70%
    speech->setRate(0.0);   // Vitesse normale
    speech->setPitch(0.0);  // Tonalité normale

    // Vérifier si des voix sont disponibles

    if (speech->availableVoices().isEmpty()) {
        qDebug() << "Aucune voix disponible pour la synthèse vocale.";
    } else {
        // Sélectionner la première voix disponible
        speech->setVoice(speech->availableVoices().first());
    }
}



MainWindow::~MainWindow()
{
    delete ui;
    delete speech; // Libérer QTextToSpeech

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

    // Récupérer l'index de la ligne sélectionnée
    QModelIndexList selectedIndexes = ui->tabV->selectionModel()->selectedRows();

    // Vérifier si une ligne est sélectionnée
    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Aucune ligne sélectionnée pour modifier un établissement.");
        return;
    }

    // Prendre la première ligne sélectionnée (colonne 0 pour l'ID)
    int id = ui->tabV->model()->data(selectedIndexes.at(0)).toInt();

    qDebug() << "ID récupéré:" << id;

    if (id == 0) {
        QMessageBox::warning(this, "Erreur", "Aucun ID valide sélectionné pour modifier un établissement.");
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
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("Veuillez saisir une valeur valide pour longitude entre 8.00 et 11.10!"), QMessageBox::Ok);
        return ;
    } else if (!latValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("Veuillez saisir une valeur valide pour latitude entre 32.80 et 37.35!"), QMessageBox::Ok);
        return ;
    } else if (!mailValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("L'email doit contenir '@' et '.'!"), QMessageBox::Ok);
        return ;
    }
    else if (!telValide) {
        QMessageBox::warning(nullptr, QObject::tr("Erreur"), QObject::tr("numéro doit étre de 8 chiffres!"), QMessageBox::Ok);
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

// supprimer tous les etablissements

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
    // Récupérer l'index de la ligne sélectionnée
    QModelIndexList selectedIndexes = ui->tabV->selectionModel()->selectedRows();

    // Vérifier si une ligne est sélectionnée
    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, QObject::tr("Erreur"), QObject::tr("Aucune ligne sélectionnée pour modifier un établissement."));
        return;
    }

    // Prendre la première ligne sélectionnée (colonne 0 pour l'ID)
    int id = ui->tabV->model()->data(selectedIndexes.at(0)).toInt();

    qDebug() << "ID récupéré:" << id;

    if (id == 0) {
        QMessageBox::warning(this, QObject::tr("Erreur"), QObject::tr("Aucun ID valide sélectionné."));
        return;
    }

    // Récupération des données
    QString nom = ui->nom->text();
    QString gouv = ui->gov3->text();
    QString longeText = ui->long_3->text();
    QString latText = ui->lat_2->text();
    QString capText = ui->cap_2->text();
    QString mail = ui->mail_2->text();
    QString tel = ui->tel_2->text();

    // Expressions régulières
    QRegularExpression regexNom("^[a-zA-ZÀ-ÖØ-öø-ÿ ]+$");
    QRegularExpression regexTel("^[0-9]+$");

    // Validation du nom
    bool nomValide = regexNom.match(nom).hasMatch();
    if (!nomValide) {
        QMessageBox::warning(this, QObject::tr("Erreur"), QObject::tr("Le nom doit contenir uniquement des lettres et des espaces!"));
        return;
    }

    // Validation du gouvernorat
    QStringList gouvernorats = {
        "Ariana", "Béja", "Ben Arous", "Bizerte", "Gabès", "Gafsa", "Jendouba", "Kairouan",
        "Kasserine", "Kébili", "Kef", "Mahdia", "Manouba", "Médenine", "Monastir", "Nabeul",
        "Sfax", "Sidi Bouzid", "Siliana", "Sousse", "Tataouine", "Tozeur", "Tunis", "Zaghouan"
    };

    bool gouvValide = gouvernorats.contains(gouv, Qt::CaseInsensitive);
    if (!gouvValide) {
        QMessageBox::warning(this, QObject::tr("Erreur"), QObject::tr("Veuillez taper un gouvernorat valide parmi les 24 gouvernorats de Tunisie!"));
        return;
    }

    // Validation de la longitude
    bool okLonge;
    float longe = longeText.toFloat(&okLonge);
    bool longeValide = okLonge && longe >= 8.00 && longe <= 11.10;
    if (!longeValide) {
        QMessageBox::warning(this, QObject::tr("Erreur"), QObject::tr("Veuillez saisir une valeur valide pour la longitude entre 8.00 et 11.10!"));
        return;
    }

    // Validation de la latitude
    bool okLat;
    float lat = latText.toFloat(&okLat);
    bool latValide = okLat && lat >= 32.80 && lat <= 37.35;
    if (!latValide) {
        QMessageBox::warning(this, QObject::tr("Erreur"), QObject::tr("Veuillez saisir une valeur valide pour la latitude entre 32.80 et 37.35!"));
        return;
    }

    // Validation de l'email
    bool mailValide = mail.contains("@") && mail.contains(".");
    if (!mailValide) {
        QMessageBox::warning(this, QObject::tr("Erreur"), QObject::tr("L'email doit contenir '@' et '.'!"));
        return;
    }

    // Validation du téléphone
    bool telValide = regexTel.match(tel).hasMatch() && tel.length() == 8;
    if (!telValide) {
        QMessageBox::warning(this, QObject::tr("Erreur"), QObject::tr("numéro doit étre de 8 chiffres."));
        return;
    }

    // Validation de la capacité
    bool okCap;
    int cap = capText.toInt(&okCap);
    bool capValide = okCap && cap > 0;
    if (!capValide) {
        QMessageBox::warning(this, QObject::tr("Erreur"), QObject::tr("La capacité doit être un nombre positif!"));
        return;
    }

    Etablissement e(nom.toStdString(), gouv.toStdString(), longe, lat, cap, mail.toStdString(), tel.toInt());

    bool test1 = e.modifier(id);

    if (test1) {
        QMessageBox::information(this, QObject::tr("Succès"), QObject::tr("L'établissement a été modifié avec succès."));
        e.afficher(ui->tabV);
        ui->nom->clear();
        ui->gov3->clear();
        ui->long_3->clear();
        ui->lat_2->clear();
        ui->cap_2->clear();
        ui->mail_2->clear();
        ui->tel_2->clear();
    } else {
        QMessageBox::warning(this, QObject::tr("Erreur"), QObject::tr("Échec de la modification."));
    }
}

// export pdf

void MainWindow::on_pdfEtab_clicked()
{
    int currentPageNumber = 1; // Compteur de pages

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

    const int margin = 200; // Reduced margins for more space
    int rowHeight = 600; // Increased row height for better readability
    const int colCountExpected = 8;
    const int pageWidth = pdfWriter.width() - 2 * margin;
    const int pageHeight = pdfWriter.height() - 2 * margin;

    // Adjusted column widths to prevent truncation and ensure readability
    QVector<int> colWidths(colCountExpected);
    colWidths[0] = pageWidth * 0.05;  // ID (small column)
    colWidths[1] = pageWidth * 0.15;  // Nom (wider for names)
    colWidths[2] = pageWidth * 0.15;  // Gouvernorat
    colWidths[3] = pageWidth * 0.12;  // Longitude
    colWidths[4] = pageWidth * 0.12;  // Latitude
    colWidths[5] = pageWidth * 0.08;  // Capa (small column)
    colWidths[6] = pageWidth * 0.22;  // Email (wider for long email addresses)
    colWidths[7] = pageWidth * 0.11;  // Tél

    const int columnSpacing = 10; // Spacing between columns
    int totalWidth = 0;
    for (int i = 0; i < colCountExpected; ++i) {
        totalWidth += colWidths[i];
    }
    totalWidth += columnSpacing * (colCountExpected - 1);

    // Scale columns if total width exceeds page width
    if (totalWidth > pageWidth) {
        float scaleFactor = static_cast<float>(pageWidth) / totalWidth;
        for (int i = 0; i < colCountExpected; ++i) {
            colWidths[i] = static_cast<int>(colWidths[i] * scaleFactor);
        }
    }

    const int fontSize = 8; // Smaller font size for body to fit more content
    const int headerFontSize = 9; // Header font size
    const int titleFontSize = 14; // Title font size
    const int headerSpacing = 100; // Spacing after headers

    QFont headerFont("Arial", headerFontSize, QFont::Bold);
    QFont bodyFont("Arial", fontSize);
    QFont titleFont("Arial", titleFontSize, QFont::Bold);

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

    // Calculate total height for scaling
    int totalHeight = rowHeight * (rowCount + 1) + headerSpacing + 500;
    float scaleFactor = 1.0;
    if (totalHeight > pageHeight) {
        scaleFactor = static_cast<float>(pageHeight) / totalHeight;
        for (int i = 0; i < colCountExpected; ++i) {
            colWidths[i] = static_cast<int>(colWidths[i] * scaleFactor);
        }
        rowHeight = static_cast<int>(rowHeight * scaleFactor);
    }

    QMap<QString, int> columnMap;
    columnMap["ID"] = -1;
    columnMap["Nom"] = -1;
    columnMap["Gouvernorat"] = -1;
    columnMap["Longitude"] = -1;
    columnMap["Latitude"] = -1;
    columnMap["Capa"] = -1;
    columnMap["Email"] = -1;
    columnMap["Tél"] = -1;

    // Map columns with flexible matching
    for (int col = 0; col < colCount; ++col) {
        QString header = model->headerData(col, Qt::Horizontal).toString();
        qDebug() << "En-tête de la colonne" << col << ":" << header;

        if (header.contains("ID", Qt::CaseInsensitive) || header == "id_etablissement") columnMap["ID"] = col;
        else if (header.contains("Nom", Qt::CaseInsensitive) || header == "nom_etablissement") columnMap["Nom"] = col;
        else if (header.contains("Gouvernorat", Qt::CaseInsensitive)) columnMap["Gouvernorat"] = col;
        else if (header.contains("Longitude", Qt::CaseInsensitive) || header.contains("long", Qt::CaseInsensitive)) columnMap["Longitude"] = col;
        else if (header.contains("Latitude", Qt::CaseInsensitive) || header.contains("lat", Qt::CaseInsensitive)) columnMap["Latitude"] = col;
        else if (header.contains("Capacité", Qt::CaseInsensitive) || header.contains("capacity", Qt::CaseInsensitive)) columnMap["Capa"] = col;
        else if (header.contains("Email", Qt::CaseInsensitive) || header.contains("mail", Qt::CaseInsensitive)) columnMap["Email"] = col;
        else if (header.contains("Téléphone", Qt::CaseInsensitive) || header.contains("phone", Qt::CaseInsensitive) || header.contains("tel", Qt::CaseInsensitive)) columnMap["Tél"] = col;
    }

    // Verify mapping
    for (const QString& key : columnMap.keys()) {
        if (columnMap[key] == -1) {
            qDebug() << "Avertissement : Colonne non mappée pour" << key;
        }
    }

    // Draw title
    painter.setFont(titleFont);
    painter.setPen(Qt::black);
    QString title = tr("Liste des Établissements - %1")
                        .arg(QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm"));
    QRect titleRect = QRect(margin, margin, pageWidth, 300);
    painter.drawText(titleRect, Qt::AlignCenter, title);
    int yPos = margin + 400;

    // Draw headers with background
    painter.setFont(headerFont);
    painter.setPen(Qt::white);
    painter.setBrush(QColor(33, 97, 140));
    QStringList headers = {"ID", "Nom", "Gouvernorat", "Longitude", "Latitude",
                           "Capa", "Email", "Tél"};
    int xPos = margin;
    for (int col = 0; col < colCountExpected; ++col) {
        QRect boundingRect = QRect(xPos, yPos, colWidths[col], rowHeight);
        painter.drawRect(boundingRect);
        painter.drawText(boundingRect.adjusted(10, 10, -10, -10), Qt::AlignCenter | Qt::TextWordWrap, headers[col]);
        xPos += colWidths[col] + columnSpacing;
    }
    painter.setBrush(Qt::NoBrush);
    yPos += rowHeight;

    // Separator line
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(margin, yPos, margin + pageWidth, yPos);
    yPos += headerSpacing;

    // Draw table body with alternating row colors
    painter.setFont(bodyFont);
    painter.setPen(QPen(Qt::black, 1));
    for (int row = 0; row < rowCount; ++row) {
        xPos = margin;
        painter.setBrush(row % 2 == 0 ? QColor(245, 245, 245) : Qt::white);
        for (int col = 0; col < colCountExpected; ++col) {
            QString header = headers[col];
            int modelCol = columnMap[header];
            QString text = "";
            if (modelCol != -1) {
                QVariant data = model->data(model->index(row, modelCol));
                text = data.toString();
            }
            QRect boundingRect = QRect(xPos, yPos, colWidths[col], rowHeight);
            painter.drawRect(boundingRect);
            painter.setPen(Qt::black);
            painter.drawText(boundingRect.adjusted(10, 10, -10, -10), Qt::AlignLeft | Qt::TextWordWrap, text);
            xPos += colWidths[col] + columnSpacing;
        }
        painter.setBrush(Qt::NoBrush);
        yPos += rowHeight;
    }

    // Draw footer
    painter.setFont(QFont("Arial", 8));
    painter.setPen(Qt::gray);
    painter.end();

    QMessageBox::information(this, tr("Succès"), tr("Le fichier PDF a été généré avec succès."));
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

    // Calculer le total pour les pourcentages

    int total = 0;
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        total += it.value();
    }

    // Créer un graphique

    QChart *chart = new QChart();
    chart->setTitle("Statistiques des établissements par gouvernorat");
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->setTheme(QChart::ChartThemeDark); // Changement pour un thème sombre et moderne

    // Créer une série de données (courbe)

    QLineSeries *series = new QLineSeries();

    int index = 0;
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        // Convertir en pourcentage
        double percentage = (it.value() * 100.0) / total;
        series->append(index, percentage);
        index++;
    }

    chart->addSeries(series);

    // Un diagramme en barres pour une meilleure visualisation

    QBarSeries *barSeries = new QBarSeries();
    QBarSet *set = new QBarSet("Nombre d'établissements");

    for (auto it = stats.begin(); it != stats.end(); ++it) {
        // Convertir en pourcentage
        double percentage = (it.value() * 100.0) / total;
        *set << percentage;
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
    axisY->setLabelFormat("%.1f%%"); // Afficher les pourcentages avec une décimale
    axisY->setTitleText("Pourcentage d'établissements par gouvernorat ");
    axisY->setRange(0, 100); // Plage de 0 à 100 pour les pourcentages
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    barSeries->attachAxis(axisY);

    // Ajouter une légende

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // Personnalisation des couleurs

    series->setColor(QColor(255, 165, 0)); // Orange vif pour la courbe
    set->setColor(QColor(50, 205, 50)); // Vert lime pour les barres

    // Personnalisation du fond

    chart->setBackgroundBrush(QBrush(QColor(30, 30, 30))); // Fond sombre pour un contraste moderne
    chart->setBackgroundRoundness(15); // Coins plus arrondis

    // Personnalisation des polices

    QFont font;
    font.setPixelSize(14); // Augmenter la taille pour plus de lisibilité
    font.setBold(true); // Police en gras pour un look plus affirmé
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

    // Zone où le stat sera affiché

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


    // Forcer un redimensionnement explicite pour s'assurer que tout est visible
    ui->tabV->resizeColumnsToContents();

    //Définir une largeur minimale pour la colonne ID si nécessaire

    ui->tabV->setColumnWidth(0, 60); // Ajustez la valeur selon vos besoins

    // Optionnel : Activer le redimensionnement interactif pour permettre à l'utilisateur d'ajuster manuellement
    header->setSectionResizeMode(QHeaderView::Interactive);

    // Assurer que le texte ne soit pas tronqué
    ui->tabV->setWordWrap(false);
    ui->tabV->setTextElideMode(Qt::ElideNone);
}

// tri etablissement

void MainWindow::on_comboBox_3_activated(int index)
{
    qDebug() << "Slot on_comboBox_3_activated déclenché avec index :" << index;

    if (index == 0) {

        // Créer un nouveau modèle pour les données triées

        QSqlQueryModel* model = new QSqlQueryModel();

        // Requête pour sélectionner toutes les colonnes et trier par GOUVERNORAT
        QString queryString = "SELECT ID_ETAB, NOM, GOUVERNORAT, LONGE, LAT, CAPACITE, MAIL, TEL "
                              "FROM ETABLISSEMENTS ORDER BY GOUVERNORAT ASC";

        // Appliquer la requête au modèle
        model->setQuery(queryString);

        // Vérifier les erreurs d'exécution de la requête
        if (model->lastError().isValid()) {
            qDebug() << "Erreur lors de l'exécution de la requête de tri :" << model->lastError().text();
            delete model;
            return;
        }

        // Définir les en-têtes du tableau
        model->setHeaderData(0, Qt::Horizontal, QString("ID"));
        model->setHeaderData(1, Qt::Horizontal, QString("Nom"));
        model->setHeaderData(2, Qt::Horizontal, QString("Gouvernorat"));
        model->setHeaderData(3, Qt::Horizontal, QString("Longitude"));
        model->setHeaderData(4, Qt::Horizontal, QString("Latitude"));
        model->setHeaderData(5, Qt::Horizontal, QString("Capacité"));
        model->setHeaderData(6, Qt::Horizontal, QString("Email"));
        model->setHeaderData(7, Qt::Horizontal, QString("Téléphone"));

        // Récupérer l'ancien modèle du tableau
        QAbstractItemModel* oldModel = ui->tabV->model();

        // Appliquer le nouveau modèle trié au tableau
        ui->tabV->setModel(model);

        // Ajuster les colonnes pour s'adapter au contenu
        QHeaderView* header = ui->tabV->horizontalHeader();
        header->setSectionResizeMode(QHeaderView::ResizeToContents);

        // Forcer un redimensionnement explicite pour s'assurer que tout est visible
        ui->tabV->resizeColumnsToContents();

        ui->tabV->setColumnWidth(0, 60); //Ajustez la valeur selon vos besoins

        // Oredimensionnement interactif pour permettre à l'utilisateur d'ajuster manuellement
        header->setSectionResizeMode(QHeaderView::Interactive);

        // Assurer que le texte ne soit pas tronqué
        ui->tabV->setWordWrap(false);
        ui->tabV->setTextElideMode(Qt::ElideNone);

        // Supprimer l'ancien modèle s'il existe et n'est pas le modèle courant
        if (oldModel && oldModel != model) {
            delete oldModel;
        }
    }
    else if (index == 1) {

        // Créer un nouveau modèle pour les données triées

        QSqlQueryModel* model = new QSqlQueryModel();

        // Requête pour sélectionner toutes les colonnes et trier par ID_ETAB
        QString queryString = "SELECT ID_ETAB, NOM, GOUVERNORAT, LONGE, LAT, CAPACITE, MAIL, TEL "
                              "FROM ETABLISSEMENTS ORDER BY ID_ETAB ASC";

        // Appliquer la requête au modèle
        model->setQuery(queryString);

        // Vérifier les erreurs d'exécution de la requête
        if (model->lastError().isValid()) {
            qDebug() << "Erreur lors de l'exécution de la requête de tri :" << model->lastError().text();
            delete model;
            return;
        }

        // Définir les en-têtes du tableau
        model->setHeaderData(0, Qt::Horizontal, QString("ID"));
        model->setHeaderData(1, Qt::Horizontal, QString("Nom"));
        model->setHeaderData(2, Qt::Horizontal, QString("Gouvernorat"));
        model->setHeaderData(3, Qt::Horizontal, QString("Longitude"));
        model->setHeaderData(4, Qt::Horizontal, QString("Latitude"));
        model->setHeaderData(5, Qt::Horizontal, QString("Capacité"));
        model->setHeaderData(6, Qt::Horizontal, QString("Email"));
        model->setHeaderData(7, Qt::Horizontal, QString("Téléphone"));

        // Récupérer l'ancien modèle du tableau

        QAbstractItemModel* oldModel = ui->tabV->model();

        // Appliquer le nouveau modèle trié au tableau

        ui->tabV->setModel(model);

        // Ajuster les colonnes pour s'adapter au contenu

        QHeaderView* header = ui->tabV->horizontalHeader();
        header->setSectionResizeMode(QHeaderView::ResizeToContents);


        // Forcer un redimensionnement explicite pour s'assurer que tout est visible
        ui->tabV->resizeColumnsToContents();

        ui->tabV->setColumnWidth(0, 60); // Ajustez la valeur selon vos besoins

        // Optionnel : Activer le redimensionnement interactif pour permettre à l'utilisateur d'ajuster manuellement
        header->setSectionResizeMode(QHeaderView::Interactive);

        // Assurer que le texte ne soit pas tronqué
        ui->tabV->setWordWrap(false);
        ui->tabV->setTextElideMode(Qt::ElideNone);


        // Supprimer l'ancien modèle s'il existe et n'est pas le modèle courant

        if (oldModel && oldModel != model) {
            delete oldModel;
        }
    }
}

// texte to speech

void MainWindow::on_textSpchBTN_clicked()
{
    // Créer une fenêtre modale
    speechDialog = new QDialog(this);
    speechDialog->setWindowTitle("Synthèse Vocale");
    speechDialog->setFixedSize(400, 200); // Taille fixe pour une apparence soignée
    speechDialog->setModal(true); // Fenêtre modale

    // Créer une mise en page verticale
    QVBoxLayout *layout = new QVBoxLayout(speechDialog);

    // Ajouter une étiquette
    QLabel *label = new QLabel("Entrez le texte à lire :", speechDialog);
    label->setStyleSheet("font-size: 14px; font-weight: bold; color: black;");
    layout->addWidget(label);

    // Ajouter le champ de saisie
    textInput = new QLineEdit(speechDialog);
    textInput->setPlaceholderText("Saisissez votre texte ici...");
    textInput->setStyleSheet(
        "QLineEdit {"
        "    border: 2px solid #d0d0d0;" // Bordure légèrement plus douce
        "    border-radius: 8px;"        // Coins plus arrondis pour un look moderne
        "    padding: 10px;"             // Plus d'espace interne pour la lisibilité
        "    font-family: 'Segoe UI', Arial, sans-serif;" // Police moderne et lisible
        "    font-size: 16px;"           // Taille augmentée pour plus de clarté
        "    font-weight: 400;"          // Poids normal pour éviter la lourdeur
        "    color: #1a1a1a;"            // Texte sombre pour un bon contraste
        "    background-color: #fafafa;" // Fond clair et propre
        "    selection-background-color: #90caf9;" // Sélection en bleu clair
        "    selection-color: #ffffff;"  // Texte sélectionné en blanc
        "}"
        "QLineEdit:focus {"
        "    border-color: #42a5f5;"    // Bordure bleue vive en focus
        "    background-color: #ffffff;" // Fond blanc pur en focus
        "    box-shadow: 0 0 5px rgba(66, 165, 245, 0.5);" // Ombre subtile
        "}"
        "QLineEdit:hover {"
        "    border-color: #90caf9;"    // Bordure légèrement bleue au survol
        "}"
        );
    layout->addWidget(textInput);

    // Ajouter un bouton pour lire le texte
    QPushButton *speakButton = new QPushButton("Lire le texte", speechDialog);
    speakButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px;"
        "    border-radius: 5px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1565C0;"
        "}"
        );
    layout->addWidget(speakButton);

    // Ajouter un bouton pour fermer
    QPushButton *closeButton = new QPushButton("Fermer", speechDialog);
    closeButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #f44336;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px;"
        "    border-radius: 5px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #d32f2f;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #b71c1c;"
        "}"
        );
    layout->addWidget(closeButton);

    // Ajouter un espaceur pour centrer les éléments
    layout->addStretch();

    // Appliquer un style à la fenêtre
    speechDialog->setStyleSheet(
        "QDialog {"
        "    background-color: #ffffff;"
        "    border: 1px solid #ccc;"
        "    border-radius: 10px;"
        "}"
        );

    // Connexions des signaux
    connect(speakButton, &QPushButton::clicked, this, &MainWindow::on_speakButtonClicked);
    connect(closeButton, &QPushButton::clicked, this, &MainWindow::on_closeSpeechDialogClicked);

    // Ajout de la logique pour la sélection dans le QTableView
    QTableView *tableView = ui->tabV; // Assurez-vous que 'tableView' est le bon nom dans votre UI
    if (tableView && tableView->selectionModel()->hasSelection()) {
        // Récupérer l'index de la ligne sélectionnée

        QModelIndexList selectedRows = tableView->selectionModel()->selectedRows();
        if (!selectedRows.isEmpty()) {
            QModelIndex index = selectedRows.at(0); // Prendre la première ligne sélectionnée
            QAbstractItemModel *model = tableView->model();

            // Récupérer les valeurs des colonnes ID_ETAB (index 0) et CAPACITE (index 5)
            QString id = model->data(model->index(index.row(), 0)).toString();
            QString capacite = model->data(model->index(index.row(), 5)).toString();

            // Générer le message
            QString message = QString("Etablissement de l'id %1 sa capacité est %2").arg(id).arg(capacite);

            // Assigner le message au champ textInput
            textInput->setText(message);

            // Arrêter la lecture en cours, si nécessaire
            if (speech->state() == QTextToSpeech::Speaking) {
                speech->stop();
            }

            // Lire le message immédiatement
            speech->say(message);
        }
    }

    // Afficher la fenêtre
    speechDialog->show();
}

void MainWindow::on_speakButtonClicked()
{
    if (!textInput || textInput->text().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez saisir un texte à lire.");
        return;
    }

    // Arrêter la lecture en cours, si nécessaire

    if (speech->state() == QTextToSpeech::Speaking) {
        speech->stop();
    }

    // Lire le texte saisi

    speech->say(textInput->text());
}

void MainWindow::on_closeSpeechDialogClicked()
{
    // Arrêter la lecture si elle est en cours

    if (speech->state() == QTextToSpeech::Speaking) {
        speech->stop();
    }

    // Fermer et supprimer la fenêtre

    if (speechDialog) {
        speechDialog->close();
        delete speechDialog;
        speechDialog = nullptr;
        textInput = nullptr;
    }
}
