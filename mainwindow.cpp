#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QRegularExpression>
#include <QDate>
#include <QTimer>
#include <QVBoxLayout>
#include <QSqlError>
#include <QScrollBar>
#include <QIcon>
#include <QPdfWriter>
#include <QPainter>
#include <QDir>
#include <QStandardPaths>
#include <QTextDocumentFragment>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QDateTime>
#include "arduino.h"
#include "examen.h"
#include "statswidgetemp.h"
#include "employe.h"

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

#include <arduino.h>
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
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , engine(new QQmlApplicationEngine(this))
    , mapWindow(nullptr)
    , speech(new QTextToSpeech(this))
    , speechDialog(nullptr)
    , textInput(nullptr)
    , arduino(new Arduino(this))
    , newPhotoSelected(false)
    , faceRecognitionActive(false)
    , consecutiveDetections(0)
    , emotionRecognitionActive(false)
    , neutralFrameCount(0)
    , lastSentiment("None")
    , cheerUpQuote("")
    , happyFrameCount(0)
    , selectedIdColis(-1)
    , currentSortColumn("ID_COLIS")
{
    ui->setupUi(this);

    // === QTextToSpeech Setup ===
    speech->setVolume(0.7);
    speech->setRate(0.0);
    speech->setPitch(0.0);
    if (!speech->availableVoices().isEmpty()) {
        speech->setVoice(speech->availableVoices().first());
    } else {
        qDebug() << "No voices available for speech synthesis.";
    }

    // === Arduino Setup ===
    QStringList ports = arduino->availablePorts();
    if (!ports.isEmpty()) {
        if (arduino->connectArduino(ports.first())) {
            qDebug() << "Arduino connected on" << ports.first();
        } else {
            QMessageBox::warning(this, "Arduino Error", "Failed to connect to Arduino.");
        }
    } else {
        QMessageBox::warning(this, "Arduino Error", "No serial ports available.");
    }
    connect(arduino, &Arduino::uidReceived, this, &MainWindow::handleUidReceived);

    // === Exam UI Initialization ===
    ui->dateInputE->setDisplayFormat("dd/MM/yyyy");
    ui->dateInputM->setDisplayFormat("dd/MM/yyyy");
    ui->trierexamen->addItems({"ID", "ID(décroissant)", "Nom Examen", "Date"});
    statusChart = new QChart();
    levelChart = new QChart();
    ui->statsWidgetExam->setChart(statusChart);
    ui->statsWidgetExam->setRenderHint(QPainter::Antialiasing);

    // === Facial Recognition ===
    cap = cv::VideoCapture();
    faceCascade.load("C:/opencv_contrib-4.9.0/install/etc/haarcascades/haarcascade_frontalface_default.xml");
    recognizer = cv::face::LBPHFaceRecognizer::create();
    recognizer->read("face_model.yml");
    user_names = {"Aziz"};
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateFrame);
    smileCascade.load("C:/opencv_contrib-4.9.0/install/etc/haarcascades/haarcascade_smile.xml");

    // === Emotion Cheer Up Messages ===
    cheerMessages << "Keep Pushing Forward!" << "You're Making Progress!" << "Stay Focused, Stay Strong!"
                  << "One Step at a Time!" << "You've Got This!" << "Turn Challenges into Wins!"
                  << "Your Effort Counts!" << "Keep Up the Momentum!";

    toggleTimer = new QTimer(this);
    connect(toggleTimer, &QTimer::timeout, this, &MainWindow::toggleEmotionRecognition);
    toggleTimer->start(60000);

    // === Chatbot Setup ===
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::on_chatReplyFinished);
    connect(ui->sendChatButton, &QPushButton::clicked, this, &MainWindow::on_sendChatButton_clicked);

    // === Table Refresh Setup ===
    refreshEmployeeTable();
    refreshExamStats();
    refreshExamTable();
    connect(ui->tableEmploye, &QTableView::clicked, this, &MainWindow::onEmployeeTableClicked);
    connect(ui->tableExamen, &QTableView::clicked, this, &MainWindow::onExamTableClicked);
    connect(ui->rechlabel, &QLineEdit::textChanged, this, &MainWindow::on_rechlabel_textChanged);
    connect(ui->trierexamen, &QComboBox::currentTextChanged, this, &MainWindow::on_trierex_clicked);

    // === Drag & Drop ToDo ===
    for (QListWidget* list : {ui->toDoList, ui->inProgressList, ui->doneList}) {
        list->setAcceptDrops(true);
        list->setDragEnabled(true);
        list->setDragDropMode(QAbstractItemView::DragDrop);
        connect(list, &QListWidget::itemChanged, this, &MainWindow::onItemChanged);
    }
    populateTodoLists();

    // === Timer for periodic refresh ===
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, [this]() {
        refreshExamTable(lastSortColumn, lastSortOrder);
    });
    refreshTimer->start(60000);

    // === UI Styling ===
    ui->cameraLabel->hide();
    ui->cameraLabel->setScaledContents(true);
    ui->emotionLabel->setScaledContents(true);
    ui->chatDisplay->setStyleSheet("QTextEdit { background-color: #ffffff; border: none; padding: 10px; font-family: 'Arial'; font-size: 14px; }");
    ui->chatInput->setStyleSheet("QLineEdit { background-color: #ffffff; border: 1px solid #cccccc; border-radius: 5px; padding: 5px; font-size: 14px; }");
    ui->sendChatButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border: none; border-radius: 5px; padding: 8px 16px; font-weight: bold; } QPushButton:hover { background-color: #45a049; }");

    // === Icons ===
    ui->ajouterExam->setIcon(QIcon("C:/.../add_icon.jpg"));
    ui->afficherExam->setIcon(QIcon("C:/.../view.png"));
    ui->statsExam->setIcon(QIcon("C:/.../stats.png"));
    ui->chatbotBTN->setIcon(QIcon("C:/.../chat.png"));
    ui->todoExam->setIcon(QIcon("C:/.../todo.png"));
    ui->genererPDF->setIcon(QIcon("C:/.../pdf.png"));
    ui->label_55->setPixmap(QPixmap("C:/.../recherche.png").scaled(32, 32, Qt::KeepAspectRatio));
    ui->label_56->setPixmap(QPixmap("C:/.../tri.png").scaled(32, 32, Qt::KeepAspectRatio));
    ui->label_66->setPixmap(QPixmap("C:/.../send.png").scaled(32, 32, Qt::KeepAspectRatio));

    //COLIS
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

//arduino
void MainWindow::handleUidReceived(const QString &uid)
{
    QString cleanedUid = uid.trimmed(); // Supprime les espaces et sauts de ligne
    qDebug() << "Received UID:" << uid;
    qDebug() << "Cleaned UID:" << cleanedUid;
    qDebug() << "Received UID (hex):" << uid.toUtf8().toHex();

    QSqlQuery query;
    query.prepare("SELECT NOMEMP, PRENOMEMP FROM EMPLOYE WHERE \"UID\" = :uid");
    query.bindValue(":uid", cleanedUid);

    if (!query.exec()) {
        qDebug() << "Query failed with error:" << query.lastError().text();
        qDebug() << "Query text:" << query.executedQuery();
        arduino->sendData("Access Denied");
        return;
    }

    if (query.next()) {
        QString name = query.value("NOMEMP").toString() + " " + query.value("PRENOMEMP").toString();
        arduino->sendData(name);
        qDebug() << "Sent employee name to Arduino:" << name;
    } else {
        qDebug() << "No rows returned for UID:" << cleanedUid;
        qDebug() << "Query executed:" << query.executedQuery();
        arduino->sendData("Access Denied");
    }

    // Connexion des signaux Arduino

    connect(arduino, &Arduino::motionDetected, this, &MainWindow::onMotionDetected);
}


void MainWindow::onMotionDetected(int motionCount)
{
    QSqlQuery updateQuery;
    updateQuery.prepare("UPDATE ETABLISSEMENTS SET MOUV = :motionCount WHERE ID_ETAB = :id");
    updateQuery.bindValue(":motionCount", motionCount);

    qDebug() << "motionCount:" << motionCount << "id:" << 602;

    updateQuery.bindValue(":id", 602);

    if (!updateQuery.exec()) {
        qDebug() << "Erreur lors de la mise à jour du champ mouv :" << updateQuery.lastError().text();
        QMessageBox::warning(this, "Erreur SQL", "Impossible de mettre à jour le nombre de mouvements.");
        return;
    }

    QString message = QString("Mouvement détecté : %1").arg(motionCount);
    QMessageBox::information(this, "Information", message);

    if (speech->state() == QTextToSpeech::Speaking) {
        speech->stop();
    }
    speech->say(message);
}


void MainWindow::populateTodoLists()
{
    ui->toDoList->clear();
    ui->inProgressList->clear();
    ui->doneList->clear();

    // Initialize counters
    int toDoCount = 0;
    int inProgressCount = 0;
    int doneCount = 0;

    QSqlQuery query;
    query.exec("SELECT ID_EXAM, NOMEXAM, TO_CHAR(DATEEXAM, 'DD/MM/YYYY') AS DATEEXAM FROM examens");
    QDate currentDate = QDate::currentDate();

    while (query.next()) {
        int idExam = query.value(0).toInt();
        QString nomExam = query.value(1).toString();
        QString dateExamStr = query.value(2).toString();
        QDate dateExam = QDate::fromString(dateExamStr, "dd/MM/yyyy");

        if (!dateExam.isValid()) {
            continue;
        }

        QString status;
        int daysToExam = currentDate.daysTo(dateExam);
        if (daysToExam < 0 || currentDate == dateExam) {
            status = "Done";
            doneCount++;
        } else if (daysToExam <= 2) {
            status = "In Progress";
            inProgressCount++;
        } else {
            status = "To Do";
            toDoCount++;
        }

        QString displayText = QString("ID: %1 | %2 | Date: %3").arg(idExam).arg(nomExam).arg(dateExamStr);
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, QString("%1|%2").arg(idExam).arg(nomExam));
        QFont font("Arial", 12, QFont::Bold);
        item->setFont(font);
        item->setForeground(QBrush(QColor(169, 169, 169))); // Light grey text for all items

        if (status == "To Do") {
            item->setForeground(QBrush(QColor(0, 128, 0))); // Green text
            item->setBackground(QBrush(QColor(0, 128, 0, 30))); // Light green background
            ui->toDoList->addItem(item);
        } else if (status == "In Progress") {
            item->setForeground(QBrush(QColor(255, 165, 0))); // Orange text
            item->setBackground(QBrush(QColor(255, 165, 0, 30))); // Light orange background
            ui->inProgressList->addItem(item);
        } else if (status == "Done") {
            item->setForeground(QBrush(QColor(128, 0, 128))); // Purple text
            item->setBackground(QBrush(QColor(128, 0, 128, 30))); // Light purple background
            ui->doneList->addItem(item);
        }
    }

    // Update the labels with the counts
    ui->toDoLabel_2->setText(QString("(%1)").arg(toDoCount));
    ui->inProgressLabel_2->setText(QString("(%1)").arg(inProgressCount));
    ui->doneLabel_2->setText(QString("(%1)").arg(doneCount));
}

// Handle item changes (after drop)
void MainWindow::onItemChanged()
{
    // Determine which list triggered the signal
    QListWidget *targetList = qobject_cast<QListWidget *>(sender());
    if (!targetList) return;

    // Get the latest item added (simplified; assumes last item is the dropped one)
    if (targetList->count() == 0) return;
    QListWidgetItem *item = targetList->item(targetList->count() - 1);
    if (!item) return;

    updateExamStatusFromDrop(targetList, item);
}

// Update exam status after drop
void MainWindow::updateExamStatusFromDrop(QListWidget *targetList, QListWidgetItem *item)
{
    QString data = item->data(Qt::UserRole).toString();
    QStringList dataParts = data.split("|");
    if (dataParts.size() < 2) return;

    int examId = dataParts[0].toInt();
    QString examName = dataParts[1];
    QString status;
    if (targetList == ui->toDoList) {
        status = "To Do";
    } else if (targetList == ui->inProgressList) {
        status = "In Progress";
    } else if (targetList == ui->doneList) {
        status = "Done";
    } else {
        return;
    }

    // Prompt user to select a new date
    QDate currentDate = QDate::currentDate();
    QDate suggestedDate;
    if (status == "To Do") {
        suggestedDate = currentDate.addDays(3); // Suggest 3 days ahead for "To Do"
    } else if (status == "In Progress") {
        suggestedDate = currentDate.addDays(1); // Suggest 1 day ahead for "In Progress"
    } else {
        suggestedDate = currentDate; // Suggest today for "Done"
    }

    // Create a dialog for date input
    QDialog dateDialog(this);
    dateDialog.setWindowTitle("Modify Exam Date");
    QVBoxLayout *layout = new QVBoxLayout(&dateDialog);

    QLabel *label = new QLabel(QString("Select new date for exam '%1' (Status: %2):").arg(examName).arg(status));
    QDateEdit *dateEdit = new QDateEdit(suggestedDate);
    dateEdit->setDisplayFormat("dd/MM/yyyy");
    dateEdit->setCalendarPopup(true);
    dateEdit->setMinimumDate(currentDate.addDays(-365)); // Allow past dates if needed
    QPushButton *confirmButton = new QPushButton("Confirm");
    QPushButton *cancelButton = new QPushButton("Cancel");

    layout->addWidget(label);
    layout->addWidget(dateEdit);
    layout->addWidget(confirmButton);
    layout->addWidget(cancelButton);

    connect(cancelButton, &QPushButton::clicked, &dateDialog, &QDialog::reject);
    connect(confirmButton, &QPushButton::clicked, &dateDialog, &QDialog::accept);

    if (dateDialog.exec() == QDialog::Rejected) {
        // If user cancels, refresh lists to revert changes
        populateTodoLists();
        return;
    }

    QDate newDate = dateEdit->date();
    QString newDateStr = newDate.toString("dd/MM/yyyy");

    // Validate the new date against the target status
    int daysToExam = currentDate.daysTo(newDate);
    bool dateIsValid = true;
    QString errorMessage;

    if (status == "To Do") {
        if (daysToExam <= 2) {
            dateIsValid = false;
            errorMessage = "Error: For 'To Do' status, the exam date must be more than 2 days from today.";
        }
    } else if (status == "In Progress") {
        if (daysToExam > 2 || daysToExam <= 0) {
            dateIsValid = false;
            errorMessage = "Error: For 'In Progress' status, the exam date must be within 1 to 2 days from today.";
        }
    } else if (status == "Done") {
        if (daysToExam > 0) {
            dateIsValid = false;
            errorMessage = "Error: For 'Done' status, the exam date must be today or in the past.";
        }
    }

    if (!dateIsValid) {
        QMessageBox::warning(this, "Invalid Date", errorMessage);
        populateTodoLists(); // Revert the drag-and-drop action
        return;
    }

    // Update the exam date in the database using TO_DATE for Oracle
    QSqlQuery query;
    query.prepare("UPDATE examens SET DATEEXAM = TO_DATE(:dateExam, 'DD/MM/YYYY') WHERE ID_EXAM = :idExam");
    query.bindValue(":dateExam", newDateStr);
    query.bindValue(":idExam", examId);

    if (query.exec()) {
        // Remove duplicates from other lists
        for (QListWidget *list : {ui->toDoList, ui->inProgressList, ui->doneList}) {
            if (list != targetList) {
                for (int i = 0; i < list->count(); ++i) {
                    QListWidgetItem *otherItem = list->item(i);
                    QString otherData = otherItem->data(Qt::UserRole).toString();
                    if (otherData.startsWith(QString("%1|").arg(examId))) {
                        delete list->takeItem(i);
                        break;
                    }
                }
            }
        }

        // Refresh the table to update the status column
        refreshExamTable(lastSortColumn, lastSortOrder);
        refreshExamStats();
        populateTodoLists(); // Refresh todo lists to reflect new date
        QMessageBox::information(this, "Success", QString("Exam '%1' date updated to %2.").arg(examName).arg(newDateStr));
    } else {
        // Log the error for debugging
        qDebug() << "SQL Error:" << query.lastError().text();
        QMessageBox::warning(this, "Error", "Failed to update exam date.\nError: " + query.lastError().text());
        populateTodoLists(); // Revert lists on failure
    }
}
void MainWindow::updateExamStatsDisplay()
{
    // Query the database to get exam statistics
    QSqlQuery query;
    int toDoCount = 0, inProgressCount = 0, doneCount = 0;
    QMap<QString, int> levelCounts; // For levels (1eme, 2eme, 3eme, 4eme)
    QMap<QString, int> subjectCounts; // For subjects (Matière)

    query.exec("SELECT TO_CHAR(DATEEXAM, 'DD/MM/YYYY') AS DATEEXAM, NIVEAUEXAM, MATIERE FROM examens");
    QDate currentDate = QDate::currentDate();

    while (query.next()) {
        QString dateExamStr = query.value(0).toString(); // DATEEXAM in DD/MM/YYYY
        QDate dateExam = QDate::fromString(dateExamStr, "dd/MM/yyyy");
        QString niveauExam = query.value(1).toString(); // NIVEAUEXAM (1eme, 2eme, etc.)
        QString matiere = query.value(2).toString();    // MATIERE

        if (!dateExam.isValid()) {
            continue;
        }

        // Determine status
        int daysToExam = currentDate.daysTo(dateExam);
        if (daysToExam < 0 || currentDate == dateExam) {
            doneCount++;
        } else if (daysToExam <= 2) {
            inProgressCount++;
        } else {
            toDoCount++;
        }

        // Update level and subject counts
        levelCounts[niveauExam]++;
        subjectCounts[matiere]++;
    }

    // Calculate total counts for percentages
    int totalStatusCount = toDoCount + inProgressCount + doneCount;
    int totalLevelCount = 0;
    for (int count : levelCounts.values()) {
        totalLevelCount += count;
    }
    int totalSubjectCount = 0;
    for (int count : subjectCounts.values()) {
        totalSubjectCount += count;
    }

    // Clear existing chart
    statusChart->removeAllSeries();

    // Rotate between the three charts
    static int chartIndex = 0;
    chartIndex = (chartIndex + 1) % 3; // Cycle through 0 (Status), 1 (Level), 2 (Subject)

    if (chartIndex == 0) {
        // 1. Pie Chart for Status
        QPieSeries *statusSeries = new QPieSeries();

        // Calculate percentages and add labels with counts and percentages
        if (totalStatusCount > 0) {
            double toDoPercent = (toDoCount * 100.0) / totalStatusCount;
            double inProgressPercent = (inProgressCount * 100.0) / totalStatusCount;
            double donePercent = (doneCount * 100.0) / totalStatusCount;

            statusSeries->append(QString("To Do: %1 (%2%)").arg(toDoCount).arg(QString::number(toDoPercent, 'f', 1)), toDoCount);
            statusSeries->append(QString("In Progress: %1 (%2%)").arg(inProgressCount).arg(QString::number(inProgressPercent, 'f', 1)), inProgressCount);
            statusSeries->append(QString("Done: %1 (%2%)").arg(doneCount).arg(QString::number(donePercent, 'f', 1)), doneCount);
        } else {
            // Handle case with no data
            statusSeries->append("To Do: 0 (0%)", 0);
            statusSeries->append("In Progress: 0 (0%)", 0);
            statusSeries->append("Done: 0 (0%)", 0);
        }

        statusChart->setTitle("Exam Status Distribution");
        statusChart->removeAxis(statusChart->axisX());
        statusChart->removeAxis(statusChart->axisY());
        statusChart->addSeries(statusSeries);

        // Optional: Customize pie slice colors for better visibility
        if (!statusSeries->slices().isEmpty()) {
            statusSeries->slices().at(0)->setColor(QColor(0, 128, 0));    // Green for To Do
            statusSeries->slices().at(1)->setColor(QColor(255, 165, 0));  // Orange for In Progress
            statusSeries->slices().at(2)->setColor(QColor(128, 0, 128));  // Purple for Done
        }
    }
    else if (chartIndex == 1) {
        // 2. Bar Chart for Level Distribution
        QBarSeries *levelSeries = new QBarSeries();
        QBarSet *levelSet = new QBarSet("Exams per Level");
        QStringList levelCategories = {"1eme", "2eme", "3eme", "4eme"};
        QStringList levelLabels;

        // Calculate percentages and create labels
        for (const QString &level : levelCategories) {
            int count = levelCounts.value(level, 0);
            double percent = totalLevelCount > 0 ? (count * 100.0) / totalLevelCount : 0.0;
            levelLabels << QString("%1: %2 (%3%)").arg(level).arg(count).arg(QString::number(percent, 'f', 1));
            *levelSet << count;
        }
        levelSeries->append(levelSet);

        statusChart->setTitle("Exams by Level");
        statusChart->removeAxis(statusChart->axisX());
        statusChart->removeAxis(statusChart->axisY());
        QBarCategoryAxis *axisX = new QBarCategoryAxis();
        axisX->append(levelLabels); // Use labels with counts and percentages
        statusChart->addAxis(axisX, Qt::AlignBottom);
        levelSeries->attachAxis(axisX);

        QValueAxis *axisY = new QValueAxis();
        axisY->setRange(0, qMax(1, levelCounts.values().isEmpty() ? 1 : *std::max_element(levelCounts.begin(), levelCounts.end())));
        statusChart->addAxis(axisY, Qt::AlignLeft);
        levelSeries->attachAxis(axisY);

        statusChart->addSeries(levelSeries);

        // Optional: Customize bar colors
        levelSet->setColor(QColor(0, 120, 180)); // Blue shade for levels
    }
    else if (chartIndex == 2) {
        // 3. Bar Chart for Subject Distribution
        QBarSeries *subjectSeries = new QBarSeries();
        QBarSet *subjectSet = new QBarSet("Exams per Subject");
        QStringList subjectCategories = subjectCounts.keys();
        QStringList subjectLabels;

        // Calculate percentages and create labels
        for (const QString &subject : subjectCategories) {
            int count = subjectCounts[subject];
            double percent = totalSubjectCount > 0 ? (count * 100.0) / totalSubjectCount : 0.0;
            subjectLabels << QString("%1: %2 (%3%)").arg(subject).arg(count).arg(QString::number(percent, 'f', 1));
            *subjectSet << count;
        }
        subjectSeries->append(subjectSet);

        statusChart->setTitle("Exams by Subject");
        statusChart->removeAxis(statusChart->axisX());
        statusChart->removeAxis(statusChart->axisY());
        QBarCategoryAxis *axisX = new QBarCategoryAxis();
        axisX->append(subjectLabels); // Use labels with counts and percentages
        statusChart->addAxis(axisX, Qt::AlignBottom);
        subjectSeries->attachAxis(axisX);

        QValueAxis *axisY = new QValueAxis();
        axisY->setRange(0, qMax(1, subjectCounts.values().isEmpty() ? 1 : *std::max_element(subjectCounts.begin(), subjectCounts.end())));
        statusChart->addAxis(axisY, Qt::AlignLeft);
        subjectSeries->attachAxis(axisY);

        statusChart->addSeries(subjectSeries);

        // Optional: Customize bar colors
        subjectSet->setColor(QColor(180, 0, 120)); // Magenta shade for subjects
    }

    // Update the statsWidgetExam with the current chart
    ui->statsWidgetExam->setChart(statusChart);
    ui->statsWidgetExam->update();
}
void MainWindow::refreshExamStats()
{
    if (!statusChart) {
        qDebug() << "statusChart is null, recreating...";
        statusChart = new QChart();
    }

    updateExamStatsDisplay(); // Update and rotate the chart

    if (ui->statsWidgetExam->chart() != statusChart) {
        ui->statsWidgetExam->setChart(statusChart);
    }

    ui->statsWidgetExam->update();
    ui->statsWidgetExam->repaint();

    // Use a timer to rotate charts every few seconds
    static QTimer *chartTimer = nullptr;
    if (!chartTimer) {
        chartTimer = new QTimer(this);
        connect(chartTimer, &QTimer::timeout, this, &MainWindow::refreshExamStats);
        chartTimer->start(5000); // Rotate every 5 seconds
    }
}
MainWindow::~MainWindow()
{
    refreshTimer->stop(); // Stop the timer
    delete refreshTimer; // Delete the timer
    delete statusChart;  // Clean up chart
    delete levelChart;   // Clean up chart
    delete networkManager; // Clean up network manager
    if (cap.isOpened()) {
        cap.release();
    }
    if (lcdTimer) {
        lcdTimer->stop();
        delete lcdTimer;
    }
    ar.close_arduino();
    delete timer;
    delete toggleTimer;
    delete ui;
    delete speech; // Libérer QTextToSpeech
}
// STATS


void MainWindow::refreshExamTable(const QString &sortColumn, const QString &sortOrder)
{

    // Get the original QSqlQueryModel from afficher()
    QSqlQueryModel *sqlModelExamen = new QSqlQueryModel();
    QString queryString = QString("SELECT ID_EXAM, NOMEXAM, NIVEAUEXAM, MATIERE, TO_CHAR(DATEEXAM, 'DD/MM/YYYY') AS DATEEXAM, DUREEEXAM, NBQUESTIONSEXAM FROM examens ORDER BY %1 %2").arg(sortColumn).arg(sortOrder);
    sqlModelExamen->setQuery(queryString);

    // Create a new editable QStandardItemModel for exams
    QStandardItemModel *modelExamen = new QStandardItemModel(this);
    modelExamen->setColumnCount(11); // 7 columns (excluding plan) + 2 for Delete/Modify
    modelExamen->setRowCount(sqlModelExamen->rowCount());

    // Define headers explicitly, omitting plan
    modelExamen->setHeaderData(0, Qt::Horizontal, tr("ID"));
    modelExamen->setHeaderData(1, Qt::Horizontal, tr("Nom Examen"));
    modelExamen->setHeaderData(2, Qt::Horizontal, tr("Niveau"));
    modelExamen->setHeaderData(3, Qt::Horizontal, tr("Matière"));
    modelExamen->setHeaderData(4, Qt::Horizontal, tr("Date"));
    modelExamen->setHeaderData(5, Qt::Horizontal, tr("Durée"));
    modelExamen->setHeaderData(6, Qt::Horizontal, tr("Nb Questions"));
    modelExamen->setHeaderData(7, Qt::Horizontal, tr("Supprimer"));
    modelExamen->setHeaderData(8, Qt::Horizontal, tr("modifier"));
    modelExamen->setHeaderData(9, Qt::Horizontal, tr("Status")); // New Status column
    modelExamen->setHeaderData(10, Qt::Horizontal, tr("Télécharger Plan"));

    QIcon deleteIcon("C:\\Users\\Med Amri\\Documents\\GitHub\\EduFlow\\res\\delete_icon.png");
    QIcon modifyIcon("C:\\Users\\Med Amri\\Documents\\GitHub\\EduFlow\\res\\edit_icon.png");
    QIcon downloadIcon("C:\\Users\\Med Amri\\Documents\\GitHub\\EduFlow\\res\\téléchargement.png"); // Ensure you have a download icon resource
    qDebug() << "Download icon is null:" << downloadIcon.isNull();
    QDate currentDate = QDate::currentDate();

    // Copy data from sqlModelExamen to the new model, skipping plan (column 7 in DB)
    for (int row = 0; row < sqlModelExamen->rowCount(); ++row) {
        // Column mappings: DB column -> Model column
        // 0: id_exam -> 0
        modelExamen->setData(modelExamen->index(row, 0),
                             sqlModelExamen->data(sqlModelExamen->index(row, 0)).toInt());
        // 1: nomExam -> 1
        modelExamen->setData(modelExamen->index(row, 1),
                             sqlModelExamen->data(sqlModelExamen->index(row, 1)));
        // 2: niveauExam -> 2
        modelExamen->setData(modelExamen->index(row, 2),
                             sqlModelExamen->data(sqlModelExamen->index(row, 2)));
        // 3: matiere -> 3
        modelExamen->setData(modelExamen->index(row, 3),
                             sqlModelExamen->data(sqlModelExamen->index(row, 3)));
        // 4: dateExam -> 4
        modelExamen->setData(modelExamen->index(row, 4),
                             sqlModelExamen->data(sqlModelExamen->index(row, 4)));
        // 5: dureeExam -> 5
        modelExamen->setData(modelExamen->index(row, 5),
                             sqlModelExamen->data(sqlModelExamen->index(row, 5)));
        // 6: nbQuestionsExam -> 6
        modelExamen->setData(modelExamen->index(row, 6),
                             sqlModelExamen->data(sqlModelExamen->index(row, 6)));

        // Add Delete and Modify text
        QStandardItem *deleteItem = new QStandardItem();
        deleteItem->setIcon(deleteIcon);
        deleteItem->setTextAlignment(Qt::AlignCenter);
        modelExamen->setItem(row, 7, deleteItem);

        QStandardItem *modifyItem = new QStandardItem();
        modifyItem->setIcon(modifyIcon);
        modifyItem->setTextAlignment(Qt::AlignCenter);
        modelExamen->setItem(row, 8, modifyItem);
        // Compute the Status based on dateExam (column 4 in the model, column 4 in DB)

        QString dateExamStr = sqlModelExamen->data(sqlModelExamen->index(row, 4)).toString();
        QDate dateExam = QDate::fromString(dateExamStr, "dd/MM/yyyy");

        QString status;

        if (!dateExam.isValid()) {
            status = "Invalid Date"; // Fallback if the date is invalid
            qDebug() << "Invalid date parsed from DB:" << dateExamStr;

        } else {
            int daysToExam = currentDate.daysTo(dateExam);
            if (daysToExam < 0 || currentDate == dateExam) {
                // If the exam date has passed or is today, mark as Done
                status = "Done";
            } else if (daysToExam <= 2) {
                // If the exam is within 2 days, mark as In Progress
                status = "In Progress";
            } else {
                // Otherwise, mark as To Do
                status = "To Do";
            }
        }
        QStandardItem *statusItem = new QStandardItem(status);
        if (status == "Done") {
            statusItem->setBackground(QBrush(QColor(128, 0, 128, 128))); // Purple with 50% transparency
        } else if (status == "In Progress") {
            statusItem->setBackground(QBrush(QColor(255, 165, 0, 128))); // Orange with 50% transparency
        } else if (status == "To Do") {
            statusItem->setBackground(QBrush(QColor(0, 128, 0, 128))); // Green with 50% transparency
        } else {
            statusItem->setBackground(QBrush(Qt::white)); // Default for "Invalid Date"
        }
        modelExamen->setItem(row, 9, statusItem);
        QStandardItem *downloadItem = new QStandardItem();
        downloadItem->setText("fichier PDF"); // Fallback text for debugging
        downloadItem->setIcon(downloadIcon);
        downloadItem->setTextAlignment(Qt::AlignCenter);
        modelExamen->setItem(row, 10, downloadItem);
    }


    // Set the new model to the exam table
    ui->tableExamen->setModel(modelExamen);

    // Adjust column widths
    refreshExamStats();
    ui->tableExamen->resizeColumnsToContents();
}

void MainWindow::onExamTableClicked(const QModelIndex &index)
{
    int row = index.row();
    int idExam = ui->tableExamen->model()->data(ui->tableExamen->model()->index(row, 0)).toInt();
    originalExamId = idExam;

    if (index.column() == 7) { // Delete column
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Delete",
                                                                  "Are you sure you want to delete this exam?",
                                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            if (exam.supprimer(idExam)) {
                refreshExamTable(); // Refresh exam table
                refreshExamStats();
                QMessageBox::information(this, "Success", "Exam deleted successfully.");
            } else {
                QMessageBox::warning(this, "Error", "Failed to delete exam.");
            }
        }
    } else if (index.column() == 8) { // Modify column
        QAbstractItemModel *modelExamen = ui->tableExamen->model();
        QString nomExam = modelExamen->data(modelExamen->index(row, 1)).toString();
        QString niveauExam = modelExamen->data(modelExamen->index(row, 2)).toString();
        QString matiere = modelExamen->data(modelExamen->index(row, 3)).toString();
        QString dateExam = modelExamen->data(modelExamen->index(row, 4)).toString();
        int dureeExam = modelExamen->data(modelExamen->index(row, 5)).toInt();
        int nbQuestionsExam = modelExamen->data(modelExamen->index(row, 6)).toInt();

        // Switch to the modify form (assuming index 2, adjust if different)
        ui->examensNavBar->setCurrentIndex(2);

        // Populate the modify form fields
        ui->IDinputM->setText(QString::number(idExam));
        ui->nomInputM->setText(nomExam);
        ui->niveauInputM->setCurrentText(niveauExam);
        ui->matiereInputM->setCurrentText(matiere);
        ui->dateInputM->setDate(QDate::fromString(dateExam, "dd/MM/yyyy"));
        ui->durreInputM->setCurrentText(QString::number(dureeExam));
        ui->nbquestionInputM->setText(QString::number(nbQuestionsExam));
        ui->planInputM->setText("Choisir un PDF"); // Reset PDF button
        planData.clear(); // Clear previous PDF data
    }else if (index.column() == 10) { // Download Plan column
        QSqlQuery query;
        query.prepare("SELECT plan FROM examens WHERE ID_EXAM = :id_exam");
        query.bindValue(":id_exam", idExam);
        if (query.exec() && query.next()) {
            QByteArray pdfData = query.value("plan").toByteArray();
            if (!pdfData.isEmpty()) {
                QString downloadsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
                if (downloadsPath.isEmpty()) {
                    downloadsPath = QDir::currentPath();
                }
                QString fileName = downloadsPath + "/exam_plan_" + QString::number(idExam) + ".pdf";
                fileName = QFileDialog::getSaveFileName(this, tr("Enregistrer le PDF"), fileName,
                                                        tr("PDF Files (*.pdf)"));
                if (!fileName.isEmpty()) {
                    QFile file(fileName);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(pdfData);
                        file.close();
                        QMessageBox::information(this, "Succès", "Fichier PDF téléchargé avec succès.");
                    } else {
                        QMessageBox::warning(this, "Erreur", "Impossible d'enregistrer le fichier.");
                    }
                }
            } else {
                QMessageBox::information(this, "Information", "Aucun PDF associé à cet examen.");
            }
        } else {
            QMessageBox::warning(this, "Erreur", "Impossible de récupérer le PDF.");
        }
    }
}
void MainWindow::on_ajouterex_clicked()
{
    // Get the form variables
    QString idStr = ui->IDInputE->text();
    QString nomExam = ui->nomInputE->text();
    QString niveauExam = ui->matiereInputE_3->currentText();
    QString matiere = ui->matiereInputE->currentText();
    QString dateExam = ui->dateInputE->text();
    QString dureeStr = ui->matiereInputE_2->currentText();
    QString nbQuestionsStr = ui->nbquestionInputE->text();
    std::vector<unsigned char> plan(planData.begin(), planData.end());
    if (idStr.isEmpty() || nomExam.isEmpty() || niveauExam.isEmpty() ||
        matiere.isEmpty() || dateExam.isEmpty() || dureeStr.isEmpty() ||
        nbQuestionsStr.isEmpty()) {
        QMessageBox::warning(this, "Erreur de saisie",
                             "Faire attention, il y'a un champ qui est vide");
        return;
    }

    // ID validation
    bool idOk;
    int id_exam = idStr.toInt(&idOk);
    if (!idOk || id_exam <= 0) {
        QMessageBox::warning(this, "Erreur de validation",
                             "L'ID doit être un entier positif.");
        return;
    }

    // NomExam validation
    QRegularExpression nameRegex("^[A-Za-zÀ-ÿ0-9 -]+$"); // Letters, numbers, spaces, hyphens, accented chars
    if (nomExam.isEmpty() || !nameRegex.match(nomExam).hasMatch()) {
        QMessageBox::warning(this, "Erreur de validation",
                             "Le nom de l'examen ne doit pas être vide et ne doit contenir que des lettres ou chiffres.");
        return;
    }

    // NiveauExam validation
    if (niveauExam.isEmpty() || !nameRegex.match(niveauExam).hasMatch()) {
        QMessageBox::warning(this, "Erreur de validation",
                             "Le niveau ne doit pas être vide et ne doit contenir que des lettres ou chiffres.");
        return;
    }

    // Matiere validation
    if (matiere.isEmpty() || !nameRegex.match(matiere).hasMatch()) {
        QMessageBox::warning(this, "Erreur de validation",
                             "La matière ne doit pas être vide et ne doit contenir que des lettres ou chiffres.");
        return;
    }

    // DateExam validation (assuming dd/MM/yyyy format)
    QDate date = QDate::fromString(dateExam, "dd/MM/yyyy");
    if (!date.isValid()) {
        QMessageBox::warning(this, "Erreur de validation",
                             "La date doit être au format jj/mm/aaaa et valide.");
        return;
    }

    // DureeExam validation
    bool dureeOk;
    int dureeExam = dureeStr.toInt(&dureeOk);
    if (!dureeOk || dureeExam <= 0) {
        QMessageBox::warning(this, "Erreur de validation",
                             "La durée doit être un nombre positif.");
        return;
    }

    // NbQuestionsExam validation
    bool nbQuestionsOk;
    int nbQuestionsExam = nbQuestionsStr.toInt(&nbQuestionsOk);
    if (!nbQuestionsOk || nbQuestionsExam <= 0) {
        QMessageBox::warning(this, "Erreur de validation",
                             "Le nombre de questions doit être un nombre positif.");
        return;
    }
    if (!nbQuestionsOk || nbQuestionsExam >= 21) {
        QMessageBox::warning(this, "Erreur de validation",
                             "Le nombre de questions doit ne peut pas dépasse 20 questions");
        return;
    }
    // Check if ID already exists in the database
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM examens WHERE ID_EXAM = :id_exam");
    checkQuery.bindValue(":id_exam", id_exam);
    if (checkQuery.exec() && checkQuery.next()) {
        int count = checkQuery.value(0).toInt();
        if (count > 0) {
            QMessageBox::warning(this, "Erreur",
                                 "Cette ID déjà existe, essayez avec une autre ID.");
            return;
        }
    }

    // Create an exam object with empty plan
    Examen e(id_exam, nomExam.toStdString(), niveauExam.toStdString(), matiere.toStdString(),
             dateExam.toStdString(), dureeExam, nbQuestionsExam, plan);

    // Add the exam to the database
    bool test = e.ajouter();
    if (test) {
        QMessageBox::information(this, tr("Exam added"),
                                 tr("Examen ajouté avec succès ✅\nClick Cancel to exit."),
                                 QMessageBox::Cancel);
        refreshExamTable();
        refreshExamStats();
        planData.clear();
        ui->planInput->setText("Choisir un PDF");
    } else {
        QMessageBox::critical(this, tr("Exam not added"),
                              tr("Erreur : Examen non ajouté ❎\nClick Cancel to exit."),
                              QMessageBox::Cancel);
    }
}
void MainWindow::on_modifierex_clicked()
{
    // Get the form variables from modify form
    QString idStr = ui->IDinputM->text();
    QString nomExam = ui->nomInputM->text();
    QString niveauExam = ui->niveauInputM->currentText();
    QString matiere = ui->matiereInputM->currentText();
    QString dateExam = ui->dateInputM->text();
    QString dureeStr = ui->durreInputM->currentText();
    QString nbQuestionsStr = ui->nbquestionInputM->text();
    std::vector<unsigned char> plan(planData.begin(), planData.end());
    if (idStr.isEmpty() || nomExam.isEmpty() || niveauExam.isEmpty() ||
        matiere.isEmpty() || dateExam.isEmpty() || dureeStr.isEmpty() ||
        nbQuestionsStr.isEmpty()) {
        QMessageBox::warning(this, "Erreur de saisie",
                             "Faire attention, il y'a un champ qui est vide");
        return;
    }

    // ID validation
    bool idOk;
    int id_exam = idStr.toInt(&idOk);
    if (id_exam != originalExamId) {
        QMessageBox::warning(this, "Modification non autorisée",
                             "Désolé, vous ne pouvez pas modifier l'ID.");
        return;
    }
    if (!idOk || id_exam <= 0) {
        QMessageBox::warning(this, "Erreur de validation",
                             "L'ID doit être un entier positif.");
        return;
    }


    // NomExam validation
    QRegularExpression nameRegex("^[A-Za-zÀ-ÿ0-9 -]+$"); // Letters, numbers, spaces, hyphens, accented chars
    if (nomExam.isEmpty() || !nameRegex.match(nomExam).hasMatch()) {
        QMessageBox::warning(this, "Erreur de validation",
                             "Le nom de l'examen ne doit pas être vide et ne doit contenir que des lettres ou chiffres.");
        return;
    }

    // NiveauExam validation
    if (niveauExam.isEmpty() || !nameRegex.match(niveauExam).hasMatch()) {
        QMessageBox::warning(this, "Erreur de validation",
                             "Le niveau ne doit pas être vide et ne doit contenir que des lettres ou chiffres.");
        return;
    }

    // Matiere validation
    if (matiere.isEmpty() || !nameRegex.match(matiere).hasMatch()) {
        QMessageBox::warning(this, "Erreur de validation",
                             "La matière ne doit pas être vide et ne doit contenir que des lettres ou chiffres.");
        return;
    }

    // DateExam validation (assuming dd/MM/yyyy format)
    QDate date = QDate::fromString(dateExam, "dd/MM/yyyy");
    if (!date.isValid()) {
        QMessageBox::warning(this, "Erreur de validation",
                             "La date doit être au format jj/mm/aaaa et valide.");
        return;
    }

    // DureeExam validation
    bool dureeOk;
    int dureeExam = dureeStr.toInt(&dureeOk);
    if (!dureeOk || dureeExam <= 0) {
        QMessageBox::warning(this, "Erreur de validation",
                             "La durée doit être un nombre positif.");
        return;
    }

    // NbQuestionsExam validation
    bool nbQuestionsOk;
    int nbQuestionsExam = nbQuestionsStr.toInt(&nbQuestionsOk);
    if (!nbQuestionsOk || nbQuestionsExam <= 0) {
        QMessageBox::warning(this, "Erreur de validation",
                             "Le nombre de questions doit être un nombre positif.");
        return;
    }
    if (!nbQuestionsOk || nbQuestionsExam >= 21) {
        QMessageBox::warning(this, "Erreur de validation",
                             "Le nombre de questions doit ne peut pas dépasse 20 questions");
        return;
    }

    // Create an exam object with updated values
    Examen e(id_exam, nomExam.toStdString(), niveauExam.toStdString(), matiere.toStdString(),
             dateExam.toStdString(), dureeExam, nbQuestionsExam, plan);

    // Modify the exam in the database
    bool test = e.modifier();
    if (test) {
        QMessageBox::information(this, tr("Exam modified"),
                                 tr("Examen modifié avec succès ✅\nClick Cancel to exit."),
                                 QMessageBox::Cancel);
        refreshExamTable();
        refreshExamStats();
        planData.clear();
        ui->planInputM->setText("Choisir un PDF");
    } else {
        QMessageBox::critical(this, tr("Exam not modified"),
                              tr("Erreur : Examen non modifié ❎\nClick Cancel to exit."),
                              QMessageBox::Cancel);
    }
}
void MainWindow::on_rechlabel_textChanged(const QString &text)
{
    QString searchText = text.trimmed();

    // Create a query model with search filter
    QSqlQueryModel *sqlModelExamen = new QSqlQueryModel();
    QString queryString;
    if (searchText.isEmpty()) {
        queryString = QString("SELECT ID_EXAM, NOMEXAM, NIVEAUEXAM, MATIERE, TO_CHAR(DATEEXAM, 'DD/MM/YYYY') AS DATEEXAM, DUREEEXAM, NBQUESTIONSEXAM FROM examens ORDER BY %1 %2")
        .arg(lastSortColumn).arg(lastSortOrder);
    } else {
        // Use explicit column selection with TO_CHAR for DATEEXAM
        queryString = QString("SELECT ID_EXAM, NOMEXAM, NIVEAUEXAM, MATIERE, TO_CHAR(DATEEXAM, 'DD/MM/YYYY') AS DATEEXAM, DUREEEXAM, NBQUESTIONSEXAM FROM examens WHERE "
                              "ID_EXAM LIKE '%%1%' OR "
                              "NOMEXAM LIKE '%%1%' OR "
                              "NIVEAUEXAM LIKE '%%1%' OR "
                              "MATIERE LIKE '%%1%' OR "
                              "TO_CHAR(DATEEXAM, 'DD/MM/YYYY') LIKE '%%1%' "
                              "ORDER BY %2 %3")
                          .arg(searchText)
                          .arg(lastSortColumn)
                          .arg(lastSortOrder);
    }
    sqlModelExamen->setQuery(queryString);

    // Create a new editable QStandardItemModel for the filtered results
    QStandardItemModel *modelExamen = new QStandardItemModel(this);
    modelExamen->setColumnCount(11); // 7 columns + Delete/Modify + Status
    modelExamen->setRowCount(sqlModelExamen->rowCount());

    // Set headers
    modelExamen->setHeaderData(0, Qt::Horizontal, tr("ID"));
    modelExamen->setHeaderData(1, Qt::Horizontal, tr("Nom Examen"));
    modelExamen->setHeaderData(2, Qt::Horizontal, tr("Niveau"));
    modelExamen->setHeaderData(3, Qt::Horizontal, tr("Matière"));
    modelExamen->setHeaderData(4, Qt::Horizontal, tr("Date"));
    modelExamen->setHeaderData(5, Qt::Horizontal, tr("Durée"));
    modelExamen->setHeaderData(6, Qt::Horizontal, tr("Nb Questions"));
    modelExamen->setHeaderData(7, Qt::Horizontal, tr("supprimer"));
    modelExamen->setHeaderData(8, Qt::Horizontal, tr("Modifier"));
    modelExamen->setHeaderData(9, Qt::Horizontal, tr("Status"));
    modelExamen->setHeaderData(10, Qt::Horizontal, tr("Télécharger Plan"));
    QIcon deleteIcon("C:\\Users\\Med Amri\\Documents\\GitHub\\EduFlow\\res\\delete_icon.png");
    QIcon modifyIcon("C:\\Users\\Med Amri\\Documents\\GitHub\\EduFlow\\res\\edit_icon.png");
    QIcon downloadIcon("C:\\Users\\Med Amri\\Documents\\GitHub\\EduFlow\\res\\téléchargement.png");
    QDate currentDate = QDate::currentDate();

    // Populate the model with filtered data
    for (int row = 0; row < sqlModelExamen->rowCount(); ++row) {
        modelExamen->setData(modelExamen->index(row, 0),
                             sqlModelExamen->data(sqlModelExamen->index(row, 0)).toInt()); // ID
        modelExamen->setData(modelExamen->index(row, 1),
                             sqlModelExamen->data(sqlModelExamen->index(row, 1))); // Nom
        modelExamen->setData(modelExamen->index(row, 2),
                             sqlModelExamen->data(sqlModelExamen->index(row, 2))); // Niveau
        modelExamen->setData(modelExamen->index(row, 3),
                             sqlModelExamen->data(sqlModelExamen->index(row, 3))); // Matière
        modelExamen->setData(modelExamen->index(row, 4),
                             sqlModelExamen->data(sqlModelExamen->index(row, 4))); // Date
        modelExamen->setData(modelExamen->index(row, 5),
                             sqlModelExamen->data(sqlModelExamen->index(row, 5))); // Durée
        modelExamen->setData(modelExamen->index(row, 6),
                             sqlModelExamen->data(sqlModelExamen->index(row, 6))); // Nb Questions

        QStandardItem *deleteItem = new QStandardItem();
        deleteItem->setIcon(deleteIcon);
        deleteItem->setTextAlignment(Qt::AlignCenter);
        modelExamen->setItem(row, 7, deleteItem);

        QStandardItem *modifyItem = new QStandardItem();
        modifyItem->setIcon(modifyIcon);
        modifyItem->setTextAlignment(Qt::AlignCenter);
        modelExamen->setItem(row, 8, modifyItem);

        // Calculate status
        QString dateExamStr = sqlModelExamen->data(sqlModelExamen->index(row, 4)).toString();
        QDate dateExam = QDate::fromString(dateExamStr, "dd/MM/yyyy");
        QString status;

        if (!dateExam.isValid()) {
            status = "Invalid Date";
            qDebug() << "Invalid date parsed from DB:" << dateExamStr;
        } else {
            int daysToExam = currentDate.daysTo(dateExam);
            if (daysToExam < 0 || currentDate == dateExam) {
                status = "Done";
            } else if (daysToExam <= 2) {
                status = "In Progress";
            } else {
                status = "To Do";
            }
        }

        QStandardItem *statusItem = new QStandardItem(status);
        if (status == "Done") {
            statusItem->setBackground(QBrush(QColor(128, 0, 128, 128))); // Purple
        } else if (status == "In Progress") {
            statusItem->setBackground(QBrush(QColor(255, 165, 0, 128))); // Orange
        } else if (status == "To Do") {
            statusItem->setBackground(QBrush(QColor(0, 128, 0, 128))); // Green
        } else {
            statusItem->setBackground(QBrush(Qt::white)); // Default for "Invalid Date"
        }
        modelExamen->setItem(row, 9, statusItem);
        QStandardItem *downloadItem = new QStandardItem();
        downloadItem->setText("fichier PDF"); // Fallback text for debugging
        downloadItem->setIcon(downloadIcon);
        downloadItem->setTextAlignment(Qt::AlignCenter);
        modelExamen->setItem(row, 10, downloadItem);
    }

    // Update the table view with the filtered model
    ui->tableExamen->setModel(modelExamen);
    ui->tableExamen->resizeColumnsToContents();
}
void MainWindow::on_trierex_clicked()
{
    QString selectedOption = ui->trierexamen->currentText(); // Get the selected option from QComboBox

    // Determine the column and order based on the selected option
    if (selectedOption == "ID(croissant)") {
        lastSortColumn = "ID_EXAM";
        lastSortOrder = "ASC";
    } else if (selectedOption == "ID(décroissant)") {
        lastSortColumn = "ID_EXAM";
        lastSortOrder = "DESC";
    } else if (selectedOption == "Nom Examen") {
        lastSortColumn = "NOMEXAM";
        lastSortOrder = "ASC"; // Default to ascending for Nom Examen
    } else if (selectedOption == "Date") {
        lastSortColumn = "DATEEXAM";
        lastSortOrder = "ASC"; // Default to ascending for Date
    } else {
        // Default sorting if no valid option is selected
        lastSortColumn = "ID_EXAM";
        lastSortOrder = "ASC";
    }

    // Refresh the table with the selected sorting
    refreshExamTable(lastSortColumn, lastSortOrder);
}


void MainWindow::on_sendChatButton_clicked()
{
    QString userMessage = ui->chatInput->text().trimmed();
    if (userMessage.isEmpty()) {
        return;
    }
    // Style user message as a blue chat bubble on the right
    QString styledUserMessage = QString(
                                    "<div style='"
                                    "display: block;"            // Block to ensure proper alignment
                                    "text-align: right;"         // Align to the right
                                    "margin: 5px 0;"             // Vertical spacing
                                    "'>"
                                    "<div style='"
                                    "display: inline-block;"     // Inline block for the bubble
                                    "background-color: #0084ff;" // Blue background like the screenshot
                                    "color: white;"              // White text
                                    "border-radius: 15px;"       // Rounded corners
                                    "padding: 10px;"             // Inner spacing
                                    "max-width: 70%;"            // Limit width
                                    "text-align: left;"          // Text aligned left within the bubble
                                    "box-shadow: 1px 1px 3px rgba(0,0,0,0.2);" // Subtle shadow
                                    "'>"
                                    "%1"
                                    "</div>"
                                    "</div>"
                                    ).arg(userMessage.toHtmlEscaped());

    ui->chatDisplay->append(styledUserMessage);

    // Perform async network check with a reliable URL
    QNetworkAccessManager *testManager = new QNetworkAccessManager(this);
    QNetworkRequest testRequest(QUrl("https://www.google.com")); // Changed to a reliable endpoint

    QNetworkReply *testReply = testManager->head(testRequest);
    connect(testReply, &QNetworkReply::sslErrors, this, [](const QList<QSslError> &errors) {
        for (const QSslError &error : errors) {
            qDebug() << "SSL Error during network check:" << error.errorString();
        }
    });

    connect(testReply, &QNetworkReply::finished, this, [this, testReply, userMessage, testManager]() {
        if (testReply->error() != QNetworkReply::NoError) {
            QString errorDetails = testReply->errorString();
            int httpStatus = testReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            // Style error message as a bot message
            QString styledErrorMessage = QString(
                                             "<div style='"
                                             "display: flex;"             // Flex to align avatar and message
                                             "align-items: flex-start;"   // Align items at the top
                                             "margin: 5px 0;"             // Vertical spacing
                                             "'>"

                                             "<div style='display: inline-block;'>"
                                             "<div style='"
                                             "font-size: 12px;"           // Smaller font for label
                                             "color: #888888;"            // Gray color for label
                                             "margin-bottom: 2px;"        // Space below the label
                                             "'>Bot</div>"
                                             "<div style='"
                                             "background-color: #ffffff;" // White background
                                             "color: #000000;"            // Black text
                                             "border-radius: 15px;"       // Rounded corners
                                             "padding: 10px;"             // Inner spacing
                                             "max-width: 70%;"            // Limit width
                                             "text-align: left;"          // Text aligned left within the bubble
                                             "border: 1px solid #e0e0e0;" // Subtle border
                                             "box-shadow: 1px 1px 3px rgba(0,0,0,0.2);" // Subtle shadow
                                             "'>"
                                             "Network error - Cannot reach the server. HTTP Status: %1, Details: %2"
                                             "</div>"
                                             "</div>"
                                             "</div>"
                                             ).arg(httpStatus).arg(errorDetails.toHtmlEscaped());

            ui->chatDisplay->append(styledErrorMessage);
            qDebug() << "Network Check Error - HTTP Status:" << httpStatus << "Details:" << errorDetails;
            testReply->deleteLater();
            testManager->deleteLater();
            return;
        }

        testReply->deleteLater();
        testManager->deleteLater();
        // Show typing indicator
        QString typingIndicator = QString(
            "<div style='"
            "display: flex;"             // Flex to align with avatar
            "align-items: center;"       // Center vertically
            "margin: 5px 0;"             // Vertical spacing
            "'>"

            "<div style='"
            "font-size: 14px;"           // Match message font size
            "color: #888888;"            // Gray color for dots
            "'>…</div>"
            "</div>"
            );
        ui->chatDisplay->append(typingIndicator);
        ui->chatDisplay->verticalScrollBar()->setValue(ui->chatDisplay->verticalScrollBar()->maximum()); // Scroll to bottom
        // Proceed with the actual API request
        QUrl url("https://generativelanguage.googleapis.com/v1/models/gemini-2.0-flash-001:generateContent?key=" + apiKey);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        connect(networkManager, &QNetworkAccessManager::sslErrors, this, [](QNetworkReply *reply, const QList<QSslError> &errors) {
            for (const QSslError &error : errors) {
                qDebug() << "SSL Error during API request:" << error.errorString();
            }
        });

        QJsonObject json;
        QJsonArray contents;
        QJsonObject message;
        message["role"] = "user";
        message["parts"] = QJsonArray{QJsonObject{{"text", userMessage}}};
        contents.append(message);
        json["contents"] = contents;

        QJsonArray safetySettings;
        QJsonObject safety1, safety2, safety3, safety4;
        safety1["category"] = "HARM_CATEGORY_HATE_SPEECH";
        safety1["threshold"] = "BLOCK_MEDIUM_AND_ABOVE";
        safety2["category"] = "HARM_CATEGORY_DANGEROUS_CONTENT";
        safety2["threshold"] = "BLOCK_MEDIUM_AND_ABOVE";
        safety3["category"] = "HARM_CATEGORY_SEXUALLY_EXPLICIT";
        safety3["threshold"] = "BLOCK_MEDIUM_AND_ABOVE";
        safety4["category"] = "HARM_CATEGORY_HARASSMENT";
        safety4["threshold"] = "BLOCK_MEDIUM_AND_ABOVE";
        safetySettings.append(safety1);
        safetySettings.append(safety2);
        safetySettings.append(safety3);
        safetySettings.append(safety4);
        json["safetySettings"] = safetySettings;

        QJsonDocument doc(json);
        QByteArray data = doc.toJson();

        networkManager->post(request, data);
    });

    ui->chatInput->clear();
}

void MainWindow::on_chatReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString errorMessage = reply->errorString();
        QString responseBody = reply->readAll();
        // Style error message as a bot message
        QString styledErrorMessage = QString(
                                         "<div style='"
                                         "display: flex;"             // Flex to align avatar and message
                                         "align-items: flex-start;"   // Align items at the top
                                         "margin: 5px 0;"             // Vertical spacing
                                         "'>"

                                         "<div style='display: inline-block;'>"
                                         "<div style='"
                                         "font-size: 12px;"           // Smaller font for label
                                         "color: #888888;"            // Gray color for label
                                         "margin-bottom: 2px;"        // Space below the label
                                         "'>Bot</div>"
                                         "<div style='"
                                         "background-color: #ffffff;" // White background
                                         "color: #000000;"            // Black text
                                         "border-radius: 15px;"       // Rounded corners
                                         "padding: 10px;"             // Inner spacing
                                         "max-width: 70%;"            // Limit width
                                         "text-align: left;"          // Text aligned left within the bubble
                                         "border: 1px solid #e0e0e0;" // Subtle border
                                         "box-shadow: 1px 1px 3px rgba(0,0,0,0.2);" // Subtle shadow
                                         "'>"
                                         "Error - HTTP Status: %1, Error: %2, Response: %3"
                                         "</div>"
                                         "</div>"
                                         "</div>"
                                         ).arg(httpStatus).arg(errorMessage.toHtmlEscaped()).arg(responseBody.toHtmlEscaped());

        ui->chatDisplay->append(styledErrorMessage);

        qDebug() << "API Request Error - HTTP Status:" << httpStatus << "Error:" << errorMessage << "Response:" << responseBody;
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject json = doc.object();

    QString botResponse;
    if (json.contains("candidates") && json["candidates"].isArray()) {
        QJsonArray candidates = json["candidates"].toArray();
        if (!candidates.isEmpty()) {
            QJsonObject candidate = candidates[0].toObject();
            if (candidate.contains("content") && candidate["content"].isObject()) {
                QJsonObject content = candidate["content"].toObject();
                if (content.contains("parts") && content["parts"].isArray()) {
                    QJsonArray parts = content["parts"].toArray();
                    if (!parts.isEmpty()) {
                        botResponse = parts[0].toObject()["text"].toString();
                    }
                }
            }
        }
    }

    if (botResponse.isEmpty()) {
        if (json.contains("error")) {
            QJsonObject errorObj = json["error"].toObject();
            QString errorMessage = errorObj["message"].toString();
            // Style API error message as a bot message
            QString styledErrorMessage = QString(
                                             "<div style='"
                                             "display: flex;"             // Flex to align avatar and message
                                             "align-items: flex-start;"   // Align items at the top
                                             "margin: 5px 0;"             // Vertical spacing
                                             "'>"

                                             "<div style='display: inline-block;'>"
                                             "<div style='"
                                             "font-size: 12px;"           // Smaller font for label
                                             "color: #888888;"            // Gray color for label
                                             "margin-bottom: 2px;"        // Space below the label
                                             "'>Bot</div>"
                                             "<div style='"
                                             "background-color: #ffffff;" // White background
                                             "color: #000000;"            // Black text
                                             "border-radius: 15px;"       // Rounded corners
                                             "padding: 10px;"             // Inner spacing
                                             "max-width: 70%;"            // Limit width
                                             "text-align: left;"          // Text aligned left within the bubble
                                             "border: 1px solid #e0e0e0;" // Subtle border
                                             "box-shadow: 1px 1px 3px rgba(0,0,0,0.2);" // Subtle shadow
                                             "'>"
                                             "API Error - %1"
                                             "</div>"
                                             "</div>"
                                             "</div>"
                                             ).arg(errorMessage.toHtmlEscaped());

            ui->chatDisplay->append(styledErrorMessage);
            qDebug() << "API Error Response:" << errorMessage;
        } else {
            // Style default error message as a bot message
            QString styledDefaultMessage = QString(
                "<div style='"
                "display: flex;"             // Flex to align avatar and message
                "align-items: flex-start;"   // Align items at the top
                "margin: 5px 0;"             // Vertical spacing
                "'>"

                "<div style='display: inline-block;'>"
                "<div style='"
                "font-size: 12px;"           // Smaller font for label
                "color: #888888;"            // Gray color for label
                "margin-bottom: 2px;"        // Space below the label
                "'>Bot</div>"
                "<div style='"
                "background-color: #ffffff;" // White background
                "color: #000000;"            // Black text
                "border-radius: 15px;"       // Rounded corners
                "padding: 10px;"             // Inner spacing
                "max-width: 70%;"            // Limit width
                "text-align: left;"          // Text aligned left within the bubble
                "border: 1px solid #e0e0e0;" // Subtle border
                "box-shadow: 1px 1px 3px rgba(0,0,0,0.2);" // Subtle shadow
                "'>"
                "Sorry, I couldn't process that."
                "</div>"
                "</div>"
                "</div>"
                );

            ui->chatDisplay->append(styledDefaultMessage);
            qDebug() << "No bot response extracted from JSON:" << json;
        }
    } else {
        lastChatbotResponse = botResponse;
        // Style successful bot response
        QString styledBotResponse = QString(
                                        "<div style='"
                                        "display: flex;"             // Flex to align avatar and message
                                        "align-items: flex-start;"   // Align items at the top
                                        "margin: 5px 0;"             // Vertical spacing
                                        "'>"

                                        "<div style='display: inline-block;'>"
                                        "<div style='"
                                        "font-size: 12px;"           // Smaller font for label
                                        "color: #888888;"            // Gray color for label
                                        "margin-bottom: 2px;"        // Space below the label
                                        "'>Bot</div>"
                                        "<div style='"
                                        "background-color: #ffffff;" // White background
                                        "color: #000000;"            // Black text
                                        "border-radius: 15px;"       // Rounded corners
                                        "padding: 10px;"             // Inner spacing
                                        "max-width: 70%;"            // Limit width
                                        "text-align: left;"          // Text aligned left within the bubble
                                        "border: 1px solid #e0e0e0;" // Subtle border
                                        "box-shadow: 1px 1px 3px rgba(0,0,0,0.2);" // Subtle shadow
                                        "'>"
                                        "%1"
                                        "</div>"
                                        "</div>"
                                        "</div>"
                                        ).arg(botResponse.toHtmlEscaped());

        ui->chatDisplay->append(styledBotResponse);
    }

    // Scroll to the bottom
    ui->chatDisplay->verticalScrollBar()->setValue(ui->chatDisplay->verticalScrollBar()->maximum());


    reply->deleteLater();
}
void MainWindow::on_planInput_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Choisir un fichier PDF"), "",
                                                    tr("PDF Files (*.pdf)"));
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.size() > 10 * 1024 * 1024) { // 10MB limit
            QMessageBox::warning(this, "Erreur", "Le fichier dépasse la taille maximale de 10 Mo.");
            return;
        }
        if (file.open(QIODevice::ReadOnly)) {
            planData = file.readAll();
            ui->planInput->setText(fileName.split('/').last());
            file.close();
        } else {
            QMessageBox::warning(this, "Erreur", "Impossible d'ouvrir le fichier.");
        }
    }
}

void MainWindow::on_planInputM_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Choisir un fichier PDF"), "",
                                                    tr("PDF Files (*.pdf)"));
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.size() > 10 * 1024 * 1024) { // 10MB limit
            QMessageBox::warning(this, "Erreur", "Le fichier dépasse la taille maximale de 10 Mo.");
            return;
        }
        if (file.open(QIODevice::ReadOnly)) {
            planData = file.readAll();
            ui->planInputM->setText(fileName.split('/').last());
            file.close();
        } else {
            QMessageBox::warning(this, "Erreur", "Impossible d'ouvrir le fichier.");
        }
    }
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
    clearInputFields();
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
    ui->statsWidgetExam->hide();
}

void MainWindow::on_afficherExam_clicked()
{
    ui->examensNavBar->setCurrentIndex(1);
    ui->statsWidgetExam->hide();
}

void MainWindow::on_modiferExam_clicked()
{
    ui->examensNavBar->setCurrentIndex(2);
    ui->statsWidgetExam->hide();
}

void MainWindow::on_statsExam_clicked()
{
    ui->examensNavBar->setCurrentIndex(3);
    // Verify the widget is still in the stack
    if (ui->examensNavBar->indexOf(ui->statsWidgetExam) == -1) {
        qDebug() << "statsWidgetExam is missing from examensNavBar!";
        ui->examensNavBar->addWidget(ui->statsWidgetExam); // Re-add it if missing
    }

    // Ensure the chart is attached
    if (!ui->statsWidgetExam->chart()) {
        qDebug() << "No chart attached to statsWidgetExam, reattaching...";
        ui->statsWidgetExam->setChart(statusChart);
    }

    // Refresh the chart data
    refreshExamStats();

    // Force redraw
    ui->statsWidgetExam->show();
    ui->statsWidgetExam->update();
    ui->statsWidgetExam->repaint();

}
void MainWindow::on_chatbotBTN_clicked()
{

    ui->examensNavBar->setCurrentIndex(4); // Assuming chatbot page is index 4
    ui->statsWidgetExam->hide();
    ui->chatDisplay->show(); // Ensure chat display is visible
    if (!timestampAdded) {
        QString currentTime = QTime::currentTime().toString("hh:mmAP"); // e.g., "10:30AM"
        QString styledTimestamp = QString(
                                      "<div style='"
                                      "text-align: center;"       // Center the timestamp
                                      "color: #888888;"           // Gray color like the screenshot
                                      "font-size: 12px;"          // Smaller font
                                      "margin-bottom: 10px;"      // Space below the timestamp
                                      "'>"
                                      "%1"
                                      "</div>"
                                      ).arg(currentTime);

        ui->chatDisplay->append(styledTimestamp);
        timestampAdded = true; // Prevent adding timestamp again
    }
}
void MainWindow::on_todoExam_clicked()
{
    ui->examensNavBar->setCurrentWidget(ui->todoPage);
    populateTodoLists();
    ui->statsWidgetExam->hide();
    ui->chatDisplay->show(); // Ensure chat display is visible
}
void MainWindow::on_genererPDF_clicked(){
    if (lastChatbotResponse.isEmpty()) {
        QMessageBox::warning(this, "No Exam Data", "No exam data available from chatbot to generate PDF.");
        return;
    }

    // Use QStandardPaths to get the Downloads directory
    QString downloadsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (downloadsPath.isEmpty()) {
        downloadsPath = QDir::currentPath();
    }

    // Create a directory for PDFs inside Downloads if it doesn't exist
    QString pdfDirPath = downloadsPath + "/ExamsPDF";
    QDir pdfDir(pdfDirPath);
    if (!pdfDir.exists()) {
        pdfDir.mkpath(".");
    }

    // Generate a unique filename using timestamp
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString pdfFilePath = pdfDirPath + "/Exam_" + timestamp + ".pdf";

    // Create PDF
    QPdfWriter pdfWriter(pdfFilePath);
    pdfWriter.setPageSize(QPageSize::A4);
    pdfWriter.setResolution(300);

    QPainter painter(&pdfWriter);
    painter.setRenderHint(QPainter::Antialiasing);

    // Set up font and layout
    QFont titleFont("Arial", 16, QFont::Bold);
    QFont bodyFont("Arial", 12);
    int margin = 100;
    int yPos = margin;

    // Draw title
    painter.setFont(titleFont);
    painter.drawText(margin, yPos, "Exam Details");
    yPos += 100;

    // Define the sentences to filter out
    QString unwantedSentence1 = "Absolument ! Voici un examen d'anglais conçu pour évaluer différents aspects de la langue.";
    QString unwantedSentence2 = "J'espère que cet exemple vous sera utile! N'hésitez pas à me poser d'autres questions si vous souhaitez";

    // Normalize the unwanted sentences (trim and simplify)
    QString normalizedUnwanted1 = unwantedSentence1.trimmed().toLower();
    QString normalizedUnwanted2 = unwantedSentence2.trimmed().toLower();

    // Split the response into lines and filter
    QStringList lines = lastChatbotResponse.split("\n");
    QStringList filteredLines;

    for (const QString &line : lines) {
        // Normalize the line for comparison: trim, remove HTML tags, convert to lowercase
        QString cleanLine = QTextDocumentFragment::fromHtml(line).toPlainText().trimmed().toLower();

        // Debug: Log the line being processed
        qDebug() << "Processing line:" << cleanLine;

        // Compare the normalized line with the unwanted sentences
        if (cleanLine != normalizedUnwanted1 && cleanLine != normalizedUnwanted2) {
            filteredLines.append(line); // Keep the original line for rendering
        } else {
            qDebug() << "Filtered out line:" << cleanLine;
        }
    }

    // Draw the filtered exam content
    painter.setFont(bodyFont);
    for (const QString &line : filteredLines) {
        if (yPos > pdfWriter.height() - margin) {
            pdfWriter.newPage();
            yPos = margin;
        }
        // Render the original line (which may contain HTML or formatting)
        QString displayLine = QTextDocumentFragment::fromHtml(line).toPlainText();
        painter.drawText(margin, yPos, displayLine);
        yPos += 50;
    }

    painter.end();

    QMessageBox::information(this, "PDF Generated", QString("PDF saved to %1").arg(pdfFilePath));
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
/*
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
}*/

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
    QTableView *tableView = ui->tabV;
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

//COLIS
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

void MainWindow::on_pushButton_ajouter_clicked() {



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
    populateTable();
}

void MainWindow::on_pdfEmp_4_clicked() {
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

void MainWindow::on_affichestat_clicked() {
    qDebug() << "Afficher Stats button clicked";


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

void MainWindow::print_to_lcd() {
    QFile logFile("crash_log.txt");
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream log(&logFile);
        log << QDateTime::currentDateTime().toString() << ": Entering print_to_lcd\n";
        try {
            static QMutex mutex;
            QMutexLocker locker(&mutex); // Ensure single execution

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

void MainWindow::read_from_arduino() {
    if (ar.getserial() && ar.getserial()->isReadable()) {
        QByteArray data = ar.read_from_arduino();
        qDebug() << "Received from Arduino:" << data;
        // Add logic to process Arduino data if needed
    }
}
