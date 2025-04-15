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
    series->setHoleSize(0.35); // Donut chart effect
    series->setPieSize(0.8);   // Adjust pie size to fit labels
    series->setLabelsVisible(true);
    series->setLabelsPosition(QPieSlice::LabelOutside); // Labels outside slices

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Role Distribution");
    chart->setAnimationOptions(QChart::AllAnimations); // Smoother animations
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setFont(QFont("Arial", 10)); // Cleaner font
    chart->setTheme(QChart::ChartThemeBlueCerulean); // Modern theme
    chart->setTitleFont(QFont("Arial", 14, QFont::Bold)); // Bold title
    chart->setBackgroundRoundness(10); // Rounded chart background
    chart->setDropShadowEnabled(true); // Subtle shadow for depth

    roleChartView = new QChartView(chart, this);
    roleChartView->setRenderHint(QPainter::Antialiasing);
    roleChartView->setMinimumSize(450, 350); // Slightly larger for clarity
}

void StatsWidgetEmp::createAgeChart() {
    QBarSeries *series = new QBarSeries();

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Age Distribution");
    chart->setAnimationOptions(QChart::AllAnimations);

    QCategoryAxis *axisX = new QCategoryAxis();
    axisX->setTitleText("Age Group");
    axisX->setLabelsFont(QFont("Arial", 10));
    axisX->setTitleFont(QFont("Arial", 12, QFont::Bold));
    axisX->setGridLineVisible(false);
    axisX->setLabelsVisible(true); // Ensure labels are visible
    axisX->setStartValue(0); // Start at 0
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Count");
    axisY->setLabelFormat("%i"); // Integer labels
    axisY->setRange(0, 10);
    axisY->setTitleFont(QFont("Arial", 12, QFont::Bold));
    axisY->setLabelsFont(QFont("Arial", 10));
    axisY->setGridLinePen(QPen(Qt::lightGray, 0.5, Qt::DashLine)); // Subtle grid
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->setTheme(QChart::ChartThemeBlueCerulean);
    chart->setTitleFont(QFont("Arial", 14, QFont::Bold));
    chart->setBackgroundRoundness(10);
    chart->setDropShadowEnabled(true);

    ageChartView = new QChartView(chart, this);
    ageChartView->setRenderHint(QPainter::Antialiasing);
    ageChartView->setMinimumSize(550, 350); // Wider to accommodate narrower bars and labels
}

void StatsWidgetEmp::updateStats() {
    qDebug() << "Updating stats...";

    // Update Role Pie Chart
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

    // Define a color palette for consistency
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
        slice->setLabelFont(QFont("Arial", 9)); // Smaller font for clarity
        slice->setLabelColor(Qt::white); // White labels for contrast
        slice->setBrush(colors[colorIndex % colors.size()]); // Assign color from palette
        slice->setBorderColor(Qt::darkGray); // Subtle border
        slice->setBorderWidth(1);
        slice->setExploded(percentage > 10); // Explode larger slices
        colorIndex++;
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
    axisX->setLabelsFont(QFont("Arial", 10));
    axisX->setTitleFont(QFont("Arial", 12, QFont::Bold));
    axisX->setGridLineVisible(false);
    axisX->setLabelsVisible(true); // Ensure labels are visible
    axisX->setStartValue(0); // Start at 0

    QSqlQuery ageQuery("SELECT dateN FROM employe");
    QMap<QString, int> ageGroups;
    QStringList categories = {"20-30", "31-40", "41-50", "51+"};
    for (const QString &group : categories) {
        ageGroups[group] = 0;
    }

    totalEmployees = 0; // Reset for age calculation
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

    // Debug counts
    qDebug() << "Age group counts:";
    for (const QString &group : categories) {
        qDebug() << group << ":" << ageGroups[group];
    }

    int maxCount = 0;
    qreal categoryIndex = 0;
    // Color palette for age groups
    QList<QColor> ageColors = {
        QColor("#1f77b4"), // Blue for 20-30
        QColor("#ff7f0e"), // Orange for 31-40
        QColor("#2ca02c"), // Green for 41-50
        QColor("#d62728")  // Red for 51+
    };

    // Create a QBarSet for each category
    for (int i = 0; i < categories.size(); ++i) {
        const QString &group = categories[i];
        int count = ageGroups[group];
        QBarSet *ageSet = new QBarSet(group);
        // Add count at the correct index, zeros elsewhere
        for (int j = 0; j < categories.size(); ++j) {
            *ageSet << (j == i ? count : 0);
        }
        ageSet->setBrush(ageColors[i % ageColors.size()]); // Unique color
        ageSet->setPen(QPen(Qt::darkGray, 1)); // Border
        ageSeries->append(ageSet);

        // Category range (e.g., [0,2), [2,4), ...)
        qreal categoryStart = categoryIndex;
        qreal categoryEnd = categoryIndex + 2.0; // 2.0-unit range for narrow bars
        axisX->append(group, categoryEnd);
        maxCount = qMax(maxCount, count);
        // Calculate percentage for label
        double percentage = (totalEmployees > 0) ? (count * 100.0 / totalEmployees) : 0.0;
        ageSet->setLabel(QString("%1%").arg(percentage, 0, 'f', 1));
        categoryIndex += 2.0; // Increment by range width
    }

    // Set axis range explicitly
    axisX->setRange(0, categories.size() * 2.0); // 0 to 8.0 for 4 categories
    chart->addAxis(axisX, Qt::AlignBottom);
    ageSeries->attachAxis(axisX);
    ageSeries->attachAxis(axisY);

    if (maxCount == 0) {
        axisY->setRange(0, 5);
    } else {
        axisY->setRange(0, maxCount + 2); // Extra space for labels
    }

    // Add percentage labels above bars, positioned at the start
    for (int i = 0; i < categories.size(); ++i) {
        int count = ageGroups[categories[i]];
        double percentage = (totalEmployees > 0) ? (count * 100.0 / totalEmployees) : 0.0;
        QChart *chart = ageChartView->chart();
        QPointF point = chart->mapToPosition(QPointF(i * 2.0, count + 0.5), ageSeries);
        QGraphicsTextItem *label = new QGraphicsTextItem(QString("%1%").arg(percentage, 0, 'f', 1));
        label->setFont(QFont("Arial", 9));
        label->setPos(point.x(), point.y() - 30); // Align to left edge
        chart->scene()->addItem(label);
    }

    // Connect hover signal for all bar sets
    for (QBarSet *ageSet : ageSeries->barSets()) {
        connect(ageSet, &QBarSet::hovered, this, [=](bool status, int index) {
            if (status) {
                qDebug() << "Bar hovered:" << ageSet->label() << "Index:" << index << "Label:" << ageSet->label();
            }
        });
    }

    ageChartView->update();

    qDebug() << "Stats updated. Role series count:" << roleSeries->count() << "Age bars:" << ageSeries->barSets().size();
    qDebug() << "Y-axis range:" << axisY->min() << "to" << axisY->max();
}
