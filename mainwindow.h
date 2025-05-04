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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>

#include "examen.h"
#include "employe.h"
#include "statswidgetemp.h"
#include "arduino.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void refreshStats();

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

    // Additional slots from second file
    void on_ajouterEtab_2_clicked();
    void on_affBtn_clicked();
    void on_checkBox_2_stateChanged(int arg1);
    void on_checkBox_stateChanged(int arg1);
    void on_ajouterEmp_8_clicked();
    void on_ajouterEmp_16_clicked();
    void on_charger_clicked();
    void on_pdfEtab_clicked();
    void on_comboBox_3_currentIndexChanged(int index);
    void on_champRecherche_3_textChanged(const QString &arg1);
    void on_textSpchBTN_clicked();
    void on_geoBTN_clicked();
    void onMapWindowClosed();
    void on_comboBox_3_activated(int index);
    void on_speakButtonClicked();
    void on_closeSpeechDialogClicked();
    void onMotionDetected(int motionCount);

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
    QNetworkAccessManager *networkManager;
    const QString apiKey = "AIzaSyDTS7x8BmQVes6cDDPrDhbIwlyeZr_EA_s";
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
};

#endif // MAINWINDOW_H
