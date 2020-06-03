#include "DynamicReadableProxy.hpp"

#include <DBusTypes.hpp>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QDebug>

namespace TCD = TuxClocker::DBus;
using namespace TuxClocker::Device;

Q_DECLARE_METATYPE(TCD::Result<QDBusVariant>)
Q_DECLARE_METATYPE(ReadableValue)
Q_DECLARE_METATYPE(ReadError)
Q_DECLARE_METATYPE(ReadResult)

ReadResult toTCResult(TCD::Result<QDBusVariant> res) {
	if (res.error)
		return static_cast<ReadError>(res.value.variant().toInt());
	
	auto type = static_cast<QMetaType::Type>(res.value.variant().type());
	auto v = res.value.variant();
	
	switch (type) {
		case QMetaType::Int:
			return v.value<int>();
		case QMetaType::UInt:
			return v.value<uint>();
		case QMetaType::Double:
			return v.value<double>();
		default:
			// TODO: indicate unhandled value
			return ReadError::UnknownError;
	}
}

DynamicReadableProxy::DynamicReadableProxy(QString path, QDBusConnection conn,
		QObject *parent) : QObject(parent),
		m_iface("org.tuxclocker", path, "org.tuxclocker.DynamicReadable", conn) {
	qDBusRegisterMetaType<TCD::Result<QDBusVariant>>();
	
	m_timer.start(1000);
	
	connect(&m_timer, &QTimer::timeout, [=] {
		QDBusReply<TCD::Result<QDBusVariant>> reply = m_iface.call("value");
		
		if (!reply.isValid())
			emit valueChanged(ReadError::UnknownError);
		else
			//qDebug() << QVariant::fromStdVariant(toTCResult(reply.value()));
			emit valueChanged(toTCResult(reply.value()));
	});
}
