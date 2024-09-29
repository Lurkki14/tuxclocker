#include "Monitor.hpp"

using namespace std;

Monitor::Monitor(QWidget *parent) : QCustomPlot(parent), color_provider() {
  setMinimumWidth(1700);
  setMinimumHeight(400);

  setLocale(QLocale(QLocale::English, QLocale::UnitedKingdom));

  setOpenGl(true);
  //setAttribute(Qt::WA_OpaquePaintEvent);
  setAntialiasedElements(QCP::AntialiasedElement::aeNone);

  setBackground(QColor(48, 56, 65));
  //setAutoAddMonitortableToLegend(false);

  xAxis->setRange(0, 60);
  yAxis->setRange(0, 5000);

  collect_metrics_timer.callOnTimeout(this, &Monitor::collect_metrics);
  collect_metrics_timer.setInterval(100);

  // With 100 it's low profile like gnome-system-monitor
  // With 20 it looks nice but may use a bit more cpu (40% of an i7-9750h core)
  replot_timer.callOnTimeout(this, &Monitor::replot);
  replot_timer.setInterval(20);

  setupYAxisTick();
  setupLessImportant();

  QObject::connect(yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(onRangeChanged(QCPRange)));
};

void Monitor::onRangeChanged(QCPRange range){
  setupYAxisTick();
}

void Monitor::setupYAxisTick(){
  QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);

  yAxis->setTicker(textTicker);

  double size = yAxis->range().size();
  double lower = yAxis->range().lower;

  textTicker->addTick(lower, "0%");
  textTicker->addTick(lower + 0.2 * size, "20%");
  textTicker->addTick(lower + 0.4 * size, "40%");
  textTicker->addTick(lower + 0.6 * size, "60%");
  textTicker->addTick(lower + 0.8 * size, "80%");
  textTicker->addTick(yAxis->range().upper, "100%");
}


void Monitor::setupLessImportant(){
  QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
  timeTicker->setTimeFormat("%m:%s:%z");
  timeTicker->setTickCount(8);
  xAxis->setTicker(timeTicker);

  xAxis->setTickLabelFont(QFont(QFont().family(), 12));
  yAxis->setTickLabelFont(QFont(QFont().family(), 12));
  xAxis->setTickLabelColor(QColor(200, 200, 200));
  yAxis->setTickLabelColor(QColor(200, 200, 200));

  xAxis2->setVisible(true);
  yAxis2->setVisible(true);
  xAxis2->setTicks(false);
  yAxis2->setTicks(false);
  xAxis2->setTickLabels(false);
  yAxis2->setTickLabels(false);

  legend->setVisible(true);
  legend->setBrush(QColor(255, 255, 255, 150));
}

double Monitor::timenow(){
  return ((double) QDateTime::currentDateTime().time().msecsSinceStartOfDay()) / 1000;
}

void Monitor::collect_initial_metrics(){
  for(auto metric : metrics){
    metric->fetchValue();
  }
}

void Monitor::collect_metrics(){
  double now = timenow();

  for(int m = 0; m < metrics.count(); m++){
    graph(m)->data()->add(QCPGraphData (now, metrics[m]->value()));

    // TODO: I dont know how to correctly tell Qt5
    // to make the dbus call async, for example by using
    // callWithCallback()
    metrics[m]->asyncFetchValue();
  }

  graph()->data()->removeBefore(now - 60);
}

void Monitor::replot(){
  rescaleAxes();

  double now = timenow();
  xAxis->setRange(now - 60, now);
  QCustomPlot::replot();
}

void Monitor::setupGraphs(){
  clearGraphs();

  for(auto metric : metrics){
    addGraph();

    graph()->setName(metric->name);
    graph()->setLineStyle(QCPGraph::lsLine);
    QPen pen(color_provider.pick());
    pen.setWidth(1);
    graph()->setPen(pen);
    graph()->setAdaptiveSampling(false);
  }
}

void Monitor::start(){
  collect_initial_metrics();
  collect_metrics_timer.start();
  replot_timer.start();
}