#include <QtDBus/QDBusInterface>
#include <QString>
#include <future>

#ifndef MetricHPP
#define MetricHPP

class Metric {
public:
  QDBusInterface *dynamic_readable_interface;
  QString dbus_obj_path;
  QString name;
  float actualValue = 0;
  std::future<void> futureValue;

  Metric(QString dbus_readable);

  void fetchName();
  void fetchValue();
  void asyncFetchValue();
  double value();
};

#endif