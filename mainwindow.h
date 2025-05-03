#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QtCharts>
#include <QQmlApplicationEngine>
#include <QtQuickWidgets/QQuickWidget>
#include <QTextToSpeech> // Ajout pour la synthèse vocale
#include <QDialog>       // Pour la fenêtre modale
#include <QLineEdit>     // Pour le champ de saisie
#include <QPushButton>   // Pour les boutons
#include <arduino.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_ajouterEmp_clicked();

    void on_afficherEmp_clicked();

    void on_modiferEmp_clicked();

    void on_statsEmp_clicked();

    void on_etablissementBTN_clicked();

    void on_employesBTN_clicked();

    void on_ajouterEtab_clicked();

    void on_modiferEtab_clicked();

    void on_afficherEtab_clicked();

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

    void onMapWindowClosed(); // gérer la fermeture de la fenêtre

    void on_comboBox_3_activated(int index);

    void on_speakButtonClicked();

    void on_closeSpeechDialogClicked();

    // Nouveaux slots pour les signaux Arduino

    void onMotionDetected(int motionCount);

private:
    Ui::MainWindow *ui;
    void setupStatsChart();
    QQmlApplicationEngine * engine;
    QObject *mapWindow; // stocker la fenêtre QML
    QTextToSpeech *speech; // Pointeur pour la synthèse vocale
    QDialog *speechDialog; // Fenêtre modale pour l'interface
    QLineEdit *textInput;  // Champ de saisie du texte
    Arduino *arduino;
};
#endif // MAINWINDOW_H
