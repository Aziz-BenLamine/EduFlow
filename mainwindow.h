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

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
