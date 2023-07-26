#pragma once

#include <Device.hpp>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QObject>
#include <QTimer>

class DynamicReadableProxy : public QObject {
public:
	DynamicReadableProxy(QString path, QDBusConnection conn, QObject *parent = nullptr);
	std::optional<QString> unit();
signals:
	void valueChanged(TuxClocker::Device::ReadResult val);
private:
	Q_OBJECT

	QDBusInterface m_iface;
	QTimer m_timer; // Emits latest value
};
