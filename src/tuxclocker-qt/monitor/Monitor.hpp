#include <QTimer>
#include <QVector>
#include <QWidget>
#include <QObject>

#include "qcustomplot.h"

#include "QColorProvider.hpp"
#include "Metric.hpp"

using namespace std;

class Monitor : public QCustomPlot {
  Q_OBJECT
public:
  QTimer collect_metrics_timer;
  QTimer replot_timer;

  QColorProvider color_provider;
  QVector<shared_ptr<Metric>> metrics;

  Monitor(QWidget *parent);

  void setupLessImportant();
  double timenow();
  void collect_initial_metrics();
  void collect_metrics();
  void replot();
  void setupGraphs();
  void start();

  void setupYAxisTick();

public slots:
  void onRangeChanged(QCPRange range);
};