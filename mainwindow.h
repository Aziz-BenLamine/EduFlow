#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QDate>
#include "connection.h"
#include "colis.h" // Include the Colis class definition

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_distributionsBTN_clicked();
    void on_ajouterColis_clicked();
    void on_afficherColis_clicked();
    void on_modiferColis_clicked();
    void on_supprimerColis_clicked();
    void on_pushButton_ajouter_clicked();
    void on_pushButton_Modifer_clicked();
    void on_tableWidget_5_itemSelectionChanged();

private:
    Ui::MainWindow *ui;
    Connection dbConnection;
    int selectedColisId;
    void populateTable();
    void populateModifierFields(const Colis &colis); // Now uses Colis class
};

#endif // MAINWINDOW_H
