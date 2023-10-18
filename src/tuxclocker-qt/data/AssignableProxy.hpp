#pragma once

// Acts as a buffer for unapplied assignables and applies them asynchronously

#include <AssignableConnection.hpp>
#include <Device.hpp>
#include <DynamicReadableConnection.hpp>
#include <memory>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QObject>
#include <variant>

namespace TC = TuxClocker;

class AssignableProxy : public QObject {
public:
	AssignableProxy(QString path, QDBusConnection conn, QObject *parent = nullptr);
	void apply();
	// Stop connection and clear current connection
	void stopConnection();
	// TODO: rename to setTargetValue for congruency
	void setValue(QVariant v) { m_value = v; }
	QVariant targetValue() { return m_value; }
	std::optional<TC::Device::AssignmentArgument> currentValue();
	QString dbusPath();
signals:
	void applied(std::optional<TC::Device::AssignmentError>);
	void connectionValueChanged(
	    std::variant<QVariant, TC::Device::AssignmentError>, QString text);
	void connectionSucceeded(QVariant);
	void connectionStarted();
	void connectionStopped(QVariant);
private:
	Q_OBJECT

	QVariant m_value;
	QVariant m_connectionValue;
	QDBusInterface *m_iface;
	DynamicReadableConnection<uint> *m_connection;
	bool m_signalConnectionSuccess;

	// This is a bit of a peril but not sure if we can store interfaces any better...
	std::shared_ptr<AssignableConnection> m_assignableConnection;

	std::optional<TC::Device::AssignmentError> doApply(const QVariant &v);
	void startConnection();
};
