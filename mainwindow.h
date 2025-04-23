#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include "examen.h"
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QNetworkAccessManager> // For HTTP requests
#include <QNetworkReply>         // For handling API responses
#include <QJsonDocument>         // For parsing JSON responses
#include <QJsonObject>           // Added for QJsonObject
#include <QJsonArray>            // Added for QJsonArray
#include <QChart>
#include <QMimeData>
#include <QListWidget>



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void refreshExamTable(const QString &sortColumn = "ID_EXAM", const QString &sortOrder = "ASC");
    void onExamTableClicked(const QModelIndex &index);
    void on_rechlabel_textChanged(const QString &text);
    void on_trierex_clicked();
    void refreshExamStats(); // Renamed for exam-specific stats refresh

    void on_ajouterEmp_clicked();
    void on_afficherEmp_clicked();
    void on_modiferEmp_clicked();
    void on_statsEmp_clicked();
    void on_etablissementBTN_clicked();
    void on_employesBTN_clicked();
    void on_ajouterEtab_clicked();
    void on_afficherEtab_clicked();
    void on_modiferEtab_clicked();
    void on_statsEtab_clicked();
    void on_distributionsBTN_clicked();
    void on_equipementsBTN_clicked();
    void on_ajouterColis_clicked();
    void on_afficherColis_clicked();
    void on_modiferColis_clicked();
    void on_statsColis_clicked();
    void on_ajouterEq_clicked();
    void on_afficherEq_clicked();
    void on_modiferEq_clicked();
    void on_statsEq_clicked();
    void on_examensBTN_clicked();
    void on_ajouterExam_clicked();
    void on_afficherExam_clicked();
    void on_modiferExam_clicked();
    void on_statsExam_clicked();
    void on_pushButton_clicked();
    void on_deconnexionBTN_clicked();
    void on_ajouterEmp_4_clicked();
    void on_ajouterex_clicked();
    void on_modifierex_clicked();
    void on_chatbotBTN_clicked();
    void on_todoExam_clicked();
    void on_sendChatButton_clicked(); // Triggered when user sends a message
    void on_chatReplyFinished(QNetworkReply *reply); // Handle API response
    void onItemChanged(); // Handle item changes after drop
    void on_genererPDF_clicked();
    void on_planInput_clicked(); // New slot for Add Exam PDF
    void on_planInputM_clicked(); // New slot for Modify Exam PDF

private:
    Ui::MainWindow *ui;
    Examen exam;
    int originalExamId;
    QString lastSortColumn = "ID_EXAM";
    QString lastSortOrder = "ASC";
    QTimer *refreshTimer;
    QChart *statusChart;  // Persistent chart for status (pie chart)
    QChart *levelChart;   // Persistent chart for level (bar chart)
    void updateExamStatsDisplay(); // Renamed for exam stats display update
    QNetworkAccessManager *networkManager; // Manages HTTP requests to the API
    const QString apiKey = "AIzaSyDTS7x8BmQVes6cDDPrDhbIwlyeZr_EA_s"; // Your API key
    void populateTodoLists(); // Populate to-do lists
    void updateExamStatusFromDrop(QListWidget *targetList, QListWidgetItem *item); // Handle drop updates
    bool timestampAdded = false; // Add this to MainWindow class
    QString lastChatbotResponse; // Add this line to declare the variable
    QByteArray planData; // Store PDF data

};

#endif // MAINWINDOW_H
