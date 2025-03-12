#include "statswidgetemp.h"
#include "employe.h"
#include <QtCharts/QChart>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarSet>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QGridLayout>
#include <QSqlQuery>
#include <QRandomGenerator>
#include <QDate>
#include <QDebug>

StatsWidgetEmp::StatsWidgetEmp(QWidget *parent) : QWidget(parent) {
    QGridLayout *layout = new QGridLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    createRoleChart();
    createAgeChart();

    layout->addWidget(roleChartView, 0, 0);
    layout->addWidget(ageChartView, 0, 1);

    setLayout(layout);

    updateStats();

    setStyleSheet(
        "StatsWidgetEmp {"
        "  background-color: #ffffff;"
        "  border-radius: 10px;"
        "  padding: 10px;"
        "}"
        "QChartView {"
        "  background-color: #f9f9f9;"
        "  border: 1px solid #e0e0e0;"
        "  border-radius: 5px;"
        "}"
        );

    qDebug() << "StatsWidgetEmp constructed with roleChartView:" << roleChartView << "and ageChartView:" << ageChartView;
}

void StatsWidgetEmp::createRoleChart() {
    QPieSeries *series = new QPieSeries();
    series->setHoleSize(0.35);
    series->setLabelsVisible(true);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Role Distribution");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->setTheme(QChart::ChartThemeLight);
    chart->setTitleBrush(QBrush(Qt::darkGray));

    roleChartView = new QChartView(chart, this);
    roleChartView->setRenderHint(QPainter::Antialiasing);
    roleChartView->setMinimumSize(400, 300);
}

void StatsWidgetEmp::createAgeChart() {
    QBarSeries *series = new QBarSeries();

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Age Distribution");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QCategoryAxis *axisX = new QCategoryAxis();
    axisX->setTitleText("Age Group");
    axisX->setLabelsAngle(0);
    axisX->setLabelsVisible(true); // Ensure labels are visible
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Count");
    axisY->setRange(0, 10);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->setTheme(QChart::ChartThemeLight);
    chart->setTitleBrush(QBrush(Qt::darkGray));

    ageChartView = new QChartView(chart, this);
    ageChartView->setRenderHint(QPainter::Antialiasing);
    ageChartView->setMinimumSize(400, 300);
}

void StatsWidgetEmp::updateStats() {
    qDebug() << "Updating stats...";

    // Update Role Pie Chart
    QPieSeries *roleSeries = qobject_cast<QPieSeries*>(roleChartView->chart()->series().at(0));
    roleSeries->clear();

    QSqlQuery roleQuery("SELECT role, COUNT(*) as count FROM employe GROUP BY role");
    while (roleQuery.next()) {
        QString role = roleQuery.value("role").toString();
        int count = roleQuery.value("count").toInt();
        QPieSlice *slice = roleSeries->append(role, count);
        slice->setLabelVisible(true);
        slice->setLabel(QString("%1 (%2)").arg(role).arg(count));
        slice->setBrush(QColor::fromHsv(QRandomGenerator::global()->bounded(360), 150, 200));
        slice->setLabelColor(Qt::darkGray);
    }

    // Update Age Bar Chart
    QBarSeries *ageSeries = qobject_cast<QBarSeries*>(ageChartView->chart()->series().at(0));
    ageSeries->clear();

    QChart *chart = ageChartView->chart();
    QCategoryAxis *oldAxisX = qobject_cast<QCategoryAxis*>(chart->axisX());
    QValueAxis *axisY = qobject_cast<QValueAxis*>(chart->axisY());

    if (oldAxisX) {
        ageSeries->detachAxis(oldAxisX);
        chart->removeAxis(oldAxisX);
        delete oldAxisX;
    }

    QCategoryAxis *axisX = new QCategoryAxis();
    axisX->setTitleText("Age Group");
    axisX->setLabelsAngle(0);
    axisX->setLabelsVisible(true); // Explicitly enable labels

    QBarSet *ageSet = new QBarSet("Employees");

    QSqlQuery ageQuery("SELECT dateN FROM employe");
    QMap<QString, int> ageGroups;
    ageGroups["20-30"] = 0;
    ageGroups["31-40"] = 0;
    ageGroups["41-50"] = 0;
    ageGroups["51+"] = 0;

    int totalEmployees = 0;
    while (ageQuery.next()) {
        QString dateN = ageQuery.value("dateN").toString();
        QDate birthDate = QDate::fromString(dateN.left(10), "yyyy-MM-dd");
        if (!birthDate.isValid()) {
            qDebug() << "Invalid date:" << dateN;
            continue;
        }

        int age = QDate::currentDate().year() - birthDate.year();
        if (age >= 20 && age <= 30) ageGroups["20-30"]++;
        else if (age <= 40) ageGroups["31-40"]++;
        else if (age <= 50) ageGroups["41-50"]++;
        else if (age > 50) ageGroups["51+"]++;
        totalEmployees++;
    }

    qDebug() << "Total employees processed:" << totalEmployees;
    qDebug() << "Age groups:" << ageGroups;

    int maxCount = 0;
    QStringList categories = ageGroups.keys();

    for (const QString &group : categories) {
        int count = ageGroups[group];
        *ageSet << count;
        maxCount = qMax(maxCount, count);
        qDebug() << "Added bar for" << group << "with count" << count;
    }

    chart->addAxis(axisX, Qt::AlignBottom);
    ageSeries->attachAxis(axisX);
    ageSeries->append(ageSet);
    ageSet->setBrush(QColor(100, 150, 255));

    if (maxCount == 0) {
        axisY->setRange(0, 5);
    } else {
        axisY->setRange(0, maxCount + 1);
    }

    ageChartView->update();

    qDebug() << "Stats updated. Role series count:" << roleSeries->count() << "Age bars:" << ageSet->count();
    qDebug() << "Y-axis range:" << axisY->min() << "to" << axisY->max();
}
