#include "statswidgetemp.h"
#include "employe.h"
#include <QtCharts/QChart>
#include <QtCharts/QPieSlice>
#include <QGridLayout>
#include <QSqlQuery>
#include <QRandomGenerator>
#include <QDebug>

StatsWidgetEmp::StatsWidgetEmp(QWidget *parent) : QWidget(parent) {
    QGridLayout *layout = new QGridLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    createRoleChart();

    layout->addWidget(roleChartView, 0, 0);

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

    qDebug() << "StatsWidgetEmp constructed with roleChartView:" << roleChartView;
}

void StatsWidgetEmp::createRoleChart() {
    QPieSeries *series = new QPieSeries();
    series->setHoleSize(0.35);
    series->setPieSize(0.8);
    series->setLabelsVisible(true);
    series->setLabelsPosition(QPieSlice::LabelOutside);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Role Distribution");
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setFont(QFont("Arial", 10));
    chart->setTheme(QChart::ChartThemeBlueCerulean);
    chart->setTitleFont(QFont("Arial", 14, QFont::Bold));
    chart->setBackgroundRoundness(10);
    chart->setDropShadowEnabled(true);

    roleChartView = new QChartView(chart, this);
    roleChartView->setRenderHint(QPainter::Antialiasing);
    roleChartView->setMinimumSize(450, 350);
}

void StatsWidgetEmp::updateStats() {
    qDebug() << "Updating stats...";

    QPieSeries *roleSeries = qobject_cast<QPieSeries*>(roleChartView->chart()->series().at(0));
    roleSeries->clear();

    QSqlQuery roleQuery("SELECT role, COUNT(*) as count FROM employe GROUP BY role");
    int totalEmployees = 0;
    QMap<QString, int> roleCounts;
    while (roleQuery.next()) {
        QString role = roleQuery.value("role").toString();
        int count = roleQuery.value("count").toInt();
        roleCounts[role] = count;
        totalEmployees += count;
    }

    QList<QColor> colors = {
        QColor("#1f77b4"), QColor("#ff7f0e"), QColor("#2ca02c"),
        QColor("#d62728"), QColor("#9467bd"), QColor("#8c564b")
    };
    int colorIndex = 0;

    for (auto it = roleCounts.constBegin(); it != roleCounts.constEnd(); ++it) {
        QString role = it.key();
        int count = it.value();
        double percentage = (totalEmployees > 0) ? (count * 100.0 / totalEmployees) : 0.0;
        QPieSlice *slice = roleSeries->append(role, count);
        slice->setLabelVisible(true);
        slice->setLabel(QString("%1\n%2 (%3%)").arg(role).arg(count).arg(percentage, 0, 'f', 1));
        slice->setLabelFont(QFont("Arial", 9));
        slice->setLabelColor(Qt::white);
        slice->setBrush(colors[colorIndex % colors.size()]);
        slice->setBorderColor(Qt::darkGray);
        slice->setBorderWidth(1);
        slice->setExploded(percentage > 10);
        colorIndex++;
    }

    qDebug() << "Stats updated. Role slices:" << roleSeries->count();
}
