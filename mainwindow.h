#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QModelIndex>
#include <QTimer>
#include <QListWidget>
#include <QDebug>
#include <QStringList>
#include <QMap>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QLineEdit>
#include <QPushButton>
#include <QDialog>
#include <QTextToSpeech>
#include <QQmlApplicationEngine>
#include <QtQuickWidgets/QQuickWidget>
#include <QtCharts>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QMimeData>

#include <QString>
#include <QTextDocument>
#include <QtPrintSupport/QPrinter>
#include <QFile>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>
#include <QTableWidget>
#include <QList>

#include "examen.h"
#include "employe.h"
#include "statswidgetemp.h"
#include "arduino.h"

/* COLIS*/
#include "colis.h"
#include "statistics_window.h"
#include <QtCore>
#include <QtGui>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void refreshStats();

    //COLIS
    struct ColisAction {
        QString timestamp;
        QString action; // "Ajouter", "Modifier", "Supprimer"
        int colisId;
        QString id_employe; // Changed or original ID_EMPLOYE
        QString id_etab; // Changed or original ID_ETAB
        QString capacite; // Changed or original CAPACITE
        QString date_arrivee; // Changed or original DATE_ARRIVEE_ESTIMEE
        QString date_sortie; // Changed or original DATE_SORTIE
        QString statut; // Changed or original STATUT
        QString details; // Additional details (e.g., old vs new values for modifications)
    };
    void appendColisAction(const QString &action, int colisId, const QString &id_employe, const QString &id_etab,
                           const QString &capacite, const QString &date_arrivee, const QString &date_sortie,
                           const QString &statut, const QString &details = "");

private slots:
    void refreshExamTable(const QString &sortColumn = "ID_EXAM", const QString &sortOrder = "ASC");
    void onExamTableClicked(const QModelIndex &index);
    void on_rechlabel_textChanged(const QString &text);
    void on_trierex_clicked();
    void refreshExamStats();
    void handleUidReceived(const QString &uid);
    void toggleEmotionRecognition();
    void refreshEmployeeTable();
    void onEmployeeTableClicked(const QModelIndex &index);
    void updateFrame();
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
    void on_sendChatButton_clicked();
    void on_chatReplyFinished(QNetworkReply *reply);
    void onItemChanged();
    void on_genererPDF_clicked();
    void on_planInput_clicked();
    void on_planInputM_clicked();
    void on_ajouterEmpBD_clicked();
    void on_photoInput_clicked();
    void on_modifierEmpBD_clicked();
    void on_champRecherche_textChanged(const QString &arg1);
    void on_pdfEmp_clicked();
    void on_photoInputM_clicked();
    void on_LOGINBTN_clicked();
    void on_LOGINFACIAL_clicked();
    void on_facialEmotion_clicked();
    void on_AjouterEquipement_clicked();

    // Equipements
    void onClarifaiReplyFinished(QNetworkReply *reply);
    void on_selectImageButton_clicked();
    void on_supprimerEq_clicked();
    void on_modifierEq_clicked();
    void on_confirmerModification_clicked();
    void on_pdfEmp_5_clicked();
    void on_comboBox_6_currentIndexChanged(int index);
    void on_champRecherche_6_textChanged(const QString &text);

    void on_utiliserEquipement_clicked();

    // Additional slots from second file
    void on_ajouterEtab_2_clicked();

    void on_checkBox_2_stateChanged(int arg1);
    void on_checkBox_stateChanged(int arg1);
    void on_ajouterEmp_8_clicked();
    void on_pdfEtab_clicked();
    void on_champRecherche_3_textChanged(const QString &arg1);
    void on_textSpchBTN_clicked();
    void on_geoBTN_clicked();
    void onMapWindowClosed();
    void on_comboBox_3_activated(int index);
    void on_speakButtonClicked();
    void on_closeSpeechDialogClicked();
    void onMotionDetected(int motionCount);

    //colis
    void on_pushButton_ajouter_clicked();
    void on_tableWidget_5_clicked(QTableWidgetItem *item);
    void on_supprimerColis_clicked();
    void on_modiferColis_2_clicked();
    void on_champRecherche_5_textChanged(const QString &text);
    void on_pdfEmp_4_clicked();
    void on_comboBox_tris_currentTextChanged(const QString &text);
    void on_recEmp_4_clicked();
    void on_affichestat_clicked();
    void on_style_clicked();
    void on_sentEMP_4_clicked();
    void print_to_lcd();
    void read_from_arduino();

private:
    Ui::MainWindow *ui;
    Examen exam;
    Employe emp;
    int originalExamId;
    QString lastSortColumn = "ID_EXAM";
    QString lastSortOrder = "ASC";
    QTimer *refreshTimer;
    QChart *statusChart;
    QChart *levelChart;

    //CHATBOTS
    QNetworkAccessManager *networkManager;
    const QString apiKey = "AIzaSyDTS7x8BmQVes6cDDPrDhbIwlyeZr_EA_s";
    QNetworkAccessManager *chatNetworkManager; // For Gemini API
    QNetworkAccessManager *clarifaiNetworkManager; // For Clarifai API


    QByteArray planData;
    Arduino *arduino;
    QString lastChatbotResponse;
    bool timestampAdded = false;
    void updateExamStatsDisplay();
    void populateTodoLists();
    void updateExamStatusFromDrop(QListWidget *targetList, QListWidgetItem *item);
    void filterEmployeeTable(const QString &searchText);
    bool newPhotoSelected;
    QString currentPhotoPath;

    // FACE ID
    cv::VideoCapture cap;
    QTimer *timer;
    cv::CascadeClassifier faceCascade;
    cv::Ptr<cv::face::LBPHFaceRecognizer> recognizer;
    std::vector<std::string> user_names;
    bool faceRecognitionActive;
    int consecutiveDetections;

    // Emotion detection
    cv::CascadeClassifier smileCascade;
    bool emotionRecognitionActive;
    QStringList cheerMessages;
    int neutralFrameCount;
    QString lastSentiment;
    QString cheerUpQuote;
    void displayCheerUpContent();
    QTimer *toggleTimer;
    int happyFrameCount;

    // Arduino UID mapping
    QMap<QString, int> uidToEmployeeId;

    // Added for QML map & speech
    QQmlApplicationEngine *engine;
    QObject *mapWindow;
    QTextToSpeech *speech;
    QDialog *speechDialog;
    QLineEdit *textInput;

    void setupStatsChart();

    //colis
    bool isPieChart = true;
    Colis colis;
    int selectedIdColis;
    QString currentSortColumn;
    QList<ColisAction> colisActions; // In-memory action log
    void clearInputFields();
    void populateTable();
    void displayColisStats();
    void on_modifyButtonClicked(int row);
    void saveActionToLogFile(const ColisAction &action); // Save action to persistent log file
    Arduino ar;
    QTimer *lcdTimer; // Timer for print_to_lcd
    QString imagePathAjout;
    QString imagePathModif;
    int currentModificationId;
    StatisticsWindow *statsWindow;
    QSerialPort *serialPort; // Declare serialPort
    bool initializeSerialPort(); // Method to initialize serial port

};

#endif // MAINWINDOW_H
