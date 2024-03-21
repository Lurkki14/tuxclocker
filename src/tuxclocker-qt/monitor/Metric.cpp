#include "Metric.hpp"

#include <QtDBus/QtDBus>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusVariant>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusReply>

#include <thread>

using namespace std;

struct BooleanValue
{
    bool my_bool;
    QDBusVariant val;
};

Q_DECLARE_METATYPE(BooleanValue);

// Marshall the MyStructure data into a D-Bus argument
QDBusArgument &operator<<(QDBusArgument &argument, const BooleanValue &mystruct)
{
    argument.beginStructure();
    argument << mystruct.my_bool << mystruct.val;
    argument.endStructure();
    return argument;
}

// Retrieve the MyStructure data from the D-Bus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, BooleanValue &mystruct)
{
    argument.beginStructure();
    argument >> mystruct.my_bool >> mystruct.val;
    argument.endStructure();
    return argument;
}

Metric::Metric(QString dbus_obj_path) : dbus_obj_path(dbus_obj_path) {
  qDBusRegisterMetaType<BooleanValue>();

  dynamic_readable_interface = new QDBusInterface(
    "org.tuxclocker",
    dbus_obj_path,
    "org.tuxclocker.DynamicReadable",
    QDBusConnection::systemBus()
  );

  if(!dynamic_readable_interface->isValid()){
    qDebug() << "WARNING: Metric got an invalid dynamic_readable_interface";
    qDebug() << "  ==> path" << dbus_obj_path;
  }

  fetchName();
};

void Metric::fetchName(){
  QDBusInterface *node_interface = new QDBusInterface(
    "org.tuxclocker",
    dbus_obj_path,
    "org.tuxclocker.Node",
    QDBusConnection::systemBus()
  );

  name = node_interface->property("name").toString();
}

void Metric::fetchValue(){
  if(dynamic_readable_interface->isValid()){
    QDBusReply<BooleanValue> reply = dynamic_readable_interface->call("value");

    if(!reply.isValid()){
      qDebug() << "WARNING: fetchValue had an invalid DBus reply";
      qDebug() << "error message: " << reply.error().message();
      qDebug() << "error name: " << reply.error().name();
    }

    // TODO: actually request double precision on DBus call method signature
    actualValue = reply.value().val.variant().toDouble();
  }
}

void Metric::asyncFetchValue(){
  // TODO: Allow cancellation. There are cases where we may be innecessary blocking
  // if we delay more time that the next time we are called.
  futureValue = async(launch::async, &Metric::fetchValue, this);
}

double Metric::value(){
  return actualValue;
}