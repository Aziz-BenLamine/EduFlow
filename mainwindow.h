#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "colis.h"
#include "connection.h"
#include <QTableWidget>
#include <QList>
#include "arduino.h"
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
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

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Method to append actions to in-memory log
    void appendColisAction(const QString &action, int colisId, const QString &id_employe, const QString &id_etab,
                           const QString &capacite, const QString &date_arrivee, const QString &date_sortie,
                           const QString &statut, const QString &details = "");

private slots:
    void on_pushButton_ajouter_clicked();
    void on_afficherColis_clicked();
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
    void on_ajouterColis_clicked();
    void print_to_lcd();
    void read_from_arduino();

private:
    Ui::MainWindow *ui;
    bool isPieChart = true;
    Colis colis;
    Connection conn;
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
};

#endif // MAINWINDOW_H
