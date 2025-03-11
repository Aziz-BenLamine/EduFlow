#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include "employe.h"

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
    void refreshEmployeeTable();
    void onEmployeeTableClicked(const QModelIndex &index);

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
    void on_ajouterEmpBD_clicked();
    void on_photoInput_clicked();

    void on_modifierEmpBD_clicked();

    void on_champRecherche_textChanged(const QString &arg1);

    void on_pdfEmp_clicked();

private:
    Ui::MainWindow *ui;
    Employe emp; // Properly declare emp as a member variable
    void filterEmployeeTable(const QString &searchText);
};
#endif // MAINWINDOW_H
