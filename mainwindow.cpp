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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , arduino(new Arduino(this)) // Initialize Arduino
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
    ui->dateInputE->setDisplayFormat("dd/MM/yyyy");
    ui->dateInputM->setDisplayFormat("dd/MM/yyyy");
    ui->trierexamen->addItem("ID");
    ui->trierexamen->addItem("ID(décroissant)");
    ui->trierexamen->addItem("Nom Examen");
    ui->trierexamen->addItem("Date");
    statusChart = new QChart();
    levelChart = new QChart();
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

    // Initialize Arduino connection
    QStringList ports = arduino->availablePorts();
    if (!ports.isEmpty()) {
        if (arduino->connectArduino(ports.first())) {
            qDebug() << "Arduino connected successfully on" << ports.first();
        } else {
            qDebug() << "Failed to connect to Arduino on" << ports.first();
            QMessageBox::warning(this, "Arduino Error", "Failed to connect to Arduino. Please check the connection.");
        }
    } else {
        qDebug() << "No serial ports available";
        QMessageBox::warning(this, "Arduino Error", "No serial ports available. Please connect the Arduino.");
    }

    // Connect Arduino UID signal
    connect(arduino, &Arduino::uidReceived, this, &MainWindow::handleUidReceived);

    ui->statsWidgetExam->setChart(statusChart);
    ui->statsWidgetExam->setRenderHint(QPainter::Antialiasing);

    ui->statsWidgetExam->setStyleSheet(
        "QChartView {"
        "   min-width: 1100px;"// Set a larger minimum width
        "   min-height: 500px;"//Set a larger minimum height
        "   max-width: 1300px;" // Optional: Set a maximum width
        "   max-height: 600px;"// Optional: Set a maximum height
        "   background-color: #ffffff;" // White background for clarity
        "   border: 1px solid #cccccc;" // Optional: Add a border
        "   margin: 10px;"       // Add some margin for spacing
        "   padding: 5px;"       // Add padding inside the widget
        "}"
        );
     ui->statsWidgetExam->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->chatDisplay->setStyleSheet(
        "QTextEdit {"
        "   background-color: #ffffff;" // White background like the screenshot
        "   border: none;"             // No border
        "   padding: 10px;"            // Inner spacing
        "   font-family: 'Arial';"     // Clean font
        "   font-size: 14px;"          // Readable size
        "}"
        );
    // Style the chat input
    ui->chatInput->setStyleSheet(
        "QLineEdit {"
        "   background-color: #ffffff;" // White background
        "   border: 1px solid #cccccc;" // Light border
        "   border-radius: 5px;"       // Slightly rounded
        "   padding: 5px;"             // Comfortable padding
        "   font-size: 14px;"
        "}"
        );

    // Style the send button
    ui->sendChatButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;" // Green button
        "   color: white;"             // White text
        "   border: none;"             // No border
        "   border-radius: 5px;"       // Rounded corners
        "   padding: 8px 16px;"        // Button size
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;" // Darker green on hover
        "}"
        );
    ui->ajouterExam->setIcon(QIcon("C:\\Users\\Med Amri\\Documents\\GitHub\\EduFlow\\res\\add_icon.jpg"));
    ui->afficherExam->setIcon(QIcon("C:\\Users\\Med Amri\\Documents\\GitHub\\EduFlow\\res\\view.png"));
    ui->statsExam->setIcon(QIcon("C:\\Users\\Med Amri\\Documents\\GitHub\\EduFlow\\res\\stats.png"));
    ui->chatbotBTN->setIcon(QIcon("C:\\Users\\Med Amri\\Documents\\GitHub\\EduFlow\\res\\chat.png"));
    ui->todoExam->setIcon(QIcon("C:\\Users\\Med Amri\\Documents\\GitHub\\EduFlow\\res\\todo.png"));
    ui->genererPDF->setIcon(QIcon("C:\\Users\\Med Amri\\Documents\\GitHub\\EduFlow\\res\\pdf.png"));
    ui->label_55->setPixmap(QPixmap("C:/Users/Med Amri/Documents/GitHub/EduFlow/res/recherche.png").scaled(32, 32, Qt::KeepAspectRatio));
     ui->label_56->setPixmap(QPixmap("C:/Users/Med Amri/Documents/GitHub/EduFlow/res/tri.png").scaled(32, 32, Qt::KeepAspectRatio));
     ui->label_66->setPixmap(QPixmap("C:/Users/Med Amri/Documents/GitHub/EduFlow/res/send.png").scaled(32, 32, Qt::KeepAspectRatio));
    refreshExamStats();
    refreshExamTable();
    connect(ui->tableExamen, &QTableView::clicked, this, &MainWindow::onExamTableClicked);
   connect(ui->rechlabel, &QLineEdit::textChanged, this, &MainWindow::on_rechlabel_textChanged); // Connect textChanged signal
    connect(ui->trierexamen, &QComboBox::currentTextChanged, this, &MainWindow::on_trierex_clicked);
   // Enable drag-and-drop
   ui->toDoList->setAcceptDrops(true);
   ui->inProgressList->setAcceptDrops(true);
   ui->doneList->setAcceptDrops(true);
   ui->toDoList->setDragEnabled(true);
   ui->inProgressList->setDragEnabled(true);
   ui->doneList->setDragEnabled(true);
   ui->toDoList->setDragDropMode(QAbstractItemView::DragDrop);
   ui->inProgressList->setDragDropMode(QAbstractItemView::DragDrop);
   ui->doneList->setDragDropMode(QAbstractItemView::DragDrop);

   // Connect itemChanged signals to detect drops
   connect(ui->toDoList, &QListWidget::itemChanged, this, &MainWindow::onItemChanged);
   connect(ui->inProgressList, &QListWidget::itemChanged, this, &MainWindow::onItemChanged);
   connect(ui->doneList, &QListWidget::itemChanged, this, &MainWindow::onItemChanged);
   // Initialize the timer
   refreshTimer = new QTimer(this);
   connect(refreshTimer, &QTimer::timeout, this, [this]() {
       refreshExamTable(lastSortColumn, lastSortOrder); // Refresh with the last sort settings
   });
   refreshTimer->start(60000); // Refresh every 60 seconds (1 minute)
   populateTodoLists();

   // Initialize chatbot
   networkManager = new QNetworkAccessManager(this);
   connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::on_chatReplyFinished);

   // Assuming UI has a QLineEdit (chatInput) and QPushButton (sendChatButton) for chatbot
   connect(ui->sendChatButton, &QPushButton::clicked, this, &MainWindow::on_sendChatButton_clicked);
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
    delete timer;
    delete toggleTimer;
    delete ui;
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


