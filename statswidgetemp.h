#ifndef STATSWIDGETEMP_H
#define STATSWIDGETEMP_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>

QT_USE_NAMESPACE

    class StatsWidgetEmp : public QWidget {
    Q_OBJECT
public:
    explicit StatsWidgetEmp(QWidget *parent = nullptr);
    void updateStats();

private:
    QChartView *roleChartView;
    QChartView *ageChartView;
    void createRoleChart();
    void createAgeChart();
};

#endif // STATSWIDGETEMP_H
