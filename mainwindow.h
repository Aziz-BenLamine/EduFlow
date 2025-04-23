/*#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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
#include "statistics_window.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_equipementsBTN_clicked();
    void on_ajouterEq_clicked();
    void on_afficherEq_clicked();
    void on_statsEq_clicked();
    void on_pushButton_clicked();
    void on_deconnexionBTN_clicked();
    void on_AjouterEquipement_clicked();
    void on_selectImageButton_clicked();
    void on_supprimerEq_clicked();
    void on_modifierEq_clicked();
    void on_confirmerModification_clicked();
    void on_pdfEmp_5_clicked();
    void on_comboBox_6_currentIndexChanged(int index);
    void on_champRecherche_6_textChanged(const QString &text);
    void onClarifaiReplyFinished(QNetworkReply *reply);

private:
    Ui::MainWindow *ui;
    QString imagePathAjout;
    QString imagePathModif;
    int currentModificationId;
    QNetworkAccessManager *networkManager;
    StatisticsWindow *statsWindow;
};

#endif // MAINWINDOW_H
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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
#include "statistics_window.h"

    QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_equipementsBTN_clicked();
    void on_ajouterEq_clicked();
    void on_afficherEq_clicked();
    void on_statsEq_clicked();
    void on_pushButton_clicked();
    void on_deconnexionBTN_clicked();
    void on_AjouterEquipement_clicked();
    void on_selectImageButton_clicked();
    void on_supprimerEq_clicked();
    void on_modifierEq_clicked();
    void on_confirmerModification_clicked();
    void on_pdfEmp_5_clicked();
    void on_comboBox_6_currentIndexChanged(int index);
    void on_champRecherche_6_textChanged(const QString &text);
    void onClarifaiReplyFinished(QNetworkReply *reply);

private:
    Ui::MainWindow *ui;
    QString imagePathAjout;
    QString imagePathModif;
    int currentModificationId;
    QNetworkAccessManager *networkManager;
    StatisticsWindow *statsWindow;
};

#endif // MAINWINDOW_H
