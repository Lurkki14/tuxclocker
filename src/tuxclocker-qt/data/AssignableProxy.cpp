#include "AssignableProxy.hpp"

#include <DBusTypes.hpp>
#include <QDBusReply>
#include <QDBusMessage>

namespace TCD = TuxClocker::DBus;

using namespace TuxClocker::Device;

Q_DECLARE_METATYPE(TCD::Result<int>)

AssignableProxy::AssignableProxy(QString path, QDBusConnection conn,
		QObject *parent) : QObject(parent) {
	qDBusRegisterMetaType<TCD::Result<int>>();
	m_iface = new QDBusInterface("org.tuxclocker", 
		path, "org.tuxclocker.Assignable", conn, this);
}
	
void AssignableProxy::apply() {
	// A value hasn't been set yet
	if (m_value.isNull())
		return;
	
	// Use QDBusVariant since otherwise tries to call with the wrong signature
	QDBusVariant dv(m_value);
	QVariant v;
	v.setValue(dv);
	QDBusReply<TCD::Result<int>> reply = m_iface->call("assign", v);
	if (reply.isValid()) {
		qDebug() << reply.value().error << reply.value().value;
		
		TCD::Result<AssignmentError> ar{reply.value().error,
			static_cast<AssignmentError>(reply.value().value)};
		qDebug("Success!");
		emit applied(ar.toOptional());
	}
	// TODO: Maybe indicate that calling dbus errored out
	else 
		emit applied(AssignmentError::UnknownError);
	// Indicate that there's no pending value to applied by making value invalid
	m_value = QVariant();
}
