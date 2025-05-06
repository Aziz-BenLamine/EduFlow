#ifndef STATISTICS_WINDOW_H
#define STATISTICS_WINDOW_H

#include <QMainWindow>
#include <QChartView>
#include <QPieSeries>

class StatisticsWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit StatisticsWindow(QWidget* parent = nullptr);
    void updatePieChart();

private:
    void createPieChart(const QMap<QString, int>& equipmentData);
    QChartView* chartView;
};

#endif // STATISTICS_WINDOW_H
