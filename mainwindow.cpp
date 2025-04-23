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
#include <QTextDocumentFragment> // Added to resolve the incomplete type error
// Include all necessary Qt Charts headers
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>

#include "examen.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
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

