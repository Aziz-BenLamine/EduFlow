#include "statistics_window.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QChart>
#include <QPieSeries>
#include <QChartView>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include <QtGlobal>

StatisticsWindow::StatisticsWindow(QWidget* parent)
    : QMainWindow(parent), chartView(nullptr) {
    setWindowTitle("Equipment Quantity Statistics");
    resize(600, 400);
    qDebug() << "StatisticsWindow created";
    updatePieChart();
}

void StatisticsWindow::updatePieChart() {
    qDebug() << "Entering updatePieChart";

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid()) {
        qDebug() << "Database is invalid";
        QMessageBox::critical(this, "Erreur", "Base de données non valide.");
        return;
    }

    if (!db.isOpen()) {
        qDebug() << "Database is not open, attempting to open";
        if (!db.open()) {
            qDebug() << "Failed to open database:" << db.lastError().text();
            QMessageBox::critical(this, "Erreur", "Impossible d'ouvrir la base de données : " + db.lastError().text());
            return;
        }
    }
    qDebug() << "Database is open";

    QSqlQuery query;
    query.prepare("SELECT NOM_EQ, QT FROM EQUIPEMENTS");
    if (!query.exec()) {
        qDebug() << "Query failed:" << query.lastError().text();
        QMessageBox::critical(this, "Erreur", "Erreur lors de la récupération des données : " + query.lastError().text());
        return;
    }
    qDebug() << "Query executed successfully";

    QMap<QString, int> equipmentData;
    while (query.next()) {
        QString name = query.value("NOM_EQ").toString();
        int quantity = query.value("QT").toInt();
        if (quantity > 0) {
            equipmentData[name] += quantity;
        }
    }
    qDebug() << "Equipment data collected:" << equipmentData;

    if (equipmentData.isEmpty()) {
        qDebug() << "No equipment data available";
        QMessageBox::information(this, "Information", "Aucune donnée d'équipement disponible pour afficher le graphique.");
        return;
    }

    createPieChart(equipmentData);
}

void StatisticsWindow::createPieChart(const QMap<QString, int>& equipmentData) {
    qDebug() << "Entering createPieChart";

    QPieSeries* series = new QPieSeries();
    int totalQuantity = 0;
    for (auto it = equipmentData.constBegin(); it != equipmentData.constEnd(); ++it) {
        series->append(it.key(), it.value());
        totalQuantity += it.value();
    }
    qDebug() << "Pie series created";

    for (QPieSlice* slice : series->slices()) {
        qreal percentage = (slice->value() / totalQuantity) * 100;
        slice->setLabel(QString("%1 (%2%)").arg(slice->label()).arg(percentage, 0, 'f', 2));
        slice->setLabelVisible(true);
    }

    QChart* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Equipment Quantities by Name");
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    qDebug() << "Chart created";

    chartView = new QChartView(chart, this);
    chartView->setRenderHint(QPainter::Antialiasing);
    qDebug() << "ChartView created";

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    layout->addWidget(chartView);
    setCentralWidget(centralWidget);
    qDebug() << "Central widget set";
}
