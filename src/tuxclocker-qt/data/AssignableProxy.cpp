#include "AssignableProxy.hpp"

#include <DBusTypes.hpp>
#include <QDBusReply>
#include <QDBusMessage>

namespace TCD = TuxClocker::DBus;

using namespace TuxClocker::Device;

Q_DECLARE_METATYPE(TCD::Result<int>)
Q_DECLARE_METATYPE(TCD::Result<QString>)
Q_DECLARE_METATYPE(TCD::Result<QDBusVariant>)

AssignableProxy::AssignableProxy(QString path, QDBusConnection conn, QObject *parent)
    : QObject(parent) {
	qDBusRegisterMetaType<TCD::Result<int>>();
	qDBusRegisterMetaType<TCD::Result<QString>>();
	qDBusRegisterMetaType<TCD::Result<QDBusVariant>>();
	m_iface =
	    new QDBusInterface("org.tuxclocker", path, "org.tuxclocker.Assignable", conn, this);
}

std::optional<AssignmentError> AssignableProxy::doApply(const QVariant &v) {
	// qDebug() << v;
	QDBusReply<TCD::Result<int>> reply = m_iface->call("assign", v);
	if (reply.isValid()) {
		TCD::Result<AssignmentError> ar{
		    reply.value().error, static_cast<AssignmentError>(reply.value().value)};
		// qDebug("Success!");
		return ar.toOptional();
	}
	// TODO: indicate dbus error
	return AssignmentError::UnknownError;
}

void AssignableProxy::apply() {
	// A value hasn't been set yet
	if (m_value.isNull())
		return;

	// Use QDBusVariant since otherwise tries to call with the wrong signature
	QDBusVariant dv(m_value);
	QVariant v;
	v.setValue(dv);
	emit applied(doApply(v));
	// Indicate that there's no pending value to applied by making value invalid
	m_value = QVariant();
}

void AssignableProxy::startConnection(std::shared_ptr<AssignableConnection> conn) {
	m_assignableConnection = conn;
	connect(conn.get(), &AssignableConnection::targetValueChanged,
	    [this](auto targetValue, auto text) {
		    auto err = doApply(targetValue);
		    if (err.has_value())
			    emit connectionValueChanged(err.value(), text);
		    else
			    emit connectionValueChanged(targetValue, text);
	    });
	// Emit started signal in case a connection emits a new value right away
	emit connectionStarted();
	m_assignableConnection->start();
}

// DBus type to the more precise C++ type
std::optional<AssignmentArgument> toAssignmentArgument(TCD::Result<QDBusVariant> res) {
	if (res.error)
		return std::nullopt;

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
		return std::nullopt;
	}
}

std::optional<AssignmentArgument> AssignableProxy::currentValue() {
	QDBusReply<TCD::Result<QDBusVariant>> reply = m_iface->call("currentValue");
	if (!reply.isValid())
		return std::nullopt;
	return toAssignmentArgument(reply.value());
}
