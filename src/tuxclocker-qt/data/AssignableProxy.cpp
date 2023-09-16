#include "AssignableProxy.hpp"

#include <DBusTypes.hpp>
#include <DynamicReadableConnection.hpp>
#include <Globals.hpp>
#include <Utils.hpp>
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

	m_connection = nullptr;
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

std::optional<DynamicReadableProxy *> fromPath(QString path) {
	DynamicReadableProxy *retval = nullptr;

	auto cb = [&](auto model, auto index, int row) {
		auto ifaceIndex = model->index(row, DeviceModel::InterfaceColumn, index);
		auto dynProxyV = ifaceIndex.data(DeviceModel::DynamicReadableProxyRole);

		if (dynProxyV.isValid()) {
			auto dynProxy = qvariant_cast<DynamicReadableProxy *>(dynProxyV);
			if (dynProxy->dbusPath() == path) {
				// TODO: stop recursing here
				retval = dynProxy;
			}
		}
		return model->index(row, DeviceModel::NameColumn, index);
	};
	Utils::traverseModel(cb, Globals::g_deviceModel);

	if (retval)
		return retval;
	return std::nullopt;
}

void AssignableProxy::apply() {
	// A value hasn't been set yet
	if (m_value.isNull())
		return;

	// Stop existing parametrization
	if (m_connection) {
		disconnect(m_connection, nullptr, nullptr, nullptr);
		m_connection->stop();
		delete m_connection;
		m_connection = nullptr;
	}

	// Parametrization
	if (m_value.canConvert<DynamicReadableConnectionData>()) {
		auto data = m_value.value<DynamicReadableConnectionData>();
		auto opt = fromPath(data.dynamicReadablePath);

		if (opt.has_value()) {
			auto proxy = opt.value();
			m_connection = new DynamicReadableConnection<uint>{*proxy, data};

			// We use this to save succeeded parametrization data
			// to settings, since keeping m_value around would affect the view
			m_connectionValue = m_value;
			startConnection();
			m_signalConnectionSuccess = true;
			m_value = QVariant();
			return;
		}
	}

	// Use QDBusVariant since otherwise tries to call with the wrong signature
	QDBusVariant dv(m_value);
	QVariant v;
	v.setValue(dv);
	emit applied(doApply(v));
	// Indicate that there's no pending value to applied by making value invalid
	m_value = QVariant();
}

void AssignableProxy::startConnection() {
	QObject::connect(m_connection, &AssignableConnection::targetValueChanged,
	    [this](auto targetValue, auto text) {
		    auto err = doApply(targetValue);
		    if (err.has_value())
			    emit connectionValueChanged(err.value(), text);
		    else {
			    emit connectionValueChanged(targetValue, text);
			    if (m_signalConnectionSuccess) {
				    emit connectionSucceeded(m_connectionValue);
				    m_signalConnectionSuccess = false;
			    }
		    }
	    });
	// Emit started signal in case a connection emits a new value right away
	emit connectionStarted();
	m_connection->start();
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

QString AssignableProxy::dbusPath() { return m_iface->path(); }
