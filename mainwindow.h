#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
