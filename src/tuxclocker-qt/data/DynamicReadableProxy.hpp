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
	QString dbusPath() { return m_iface.path(); }
	void suspend() { m_timer.stop(); }
	void resume() {
		if (!m_timer.isActive())
			m_timer.start(m_updateInterval);
	}
signals:
	void valueChanged(TuxClocker::Device::ReadResult val);
private:
	Q_OBJECT

	static constexpr int m_updateInterval = 1000;
	QDBusInterface m_iface;
	QTimer m_timer; // Emits latest value
};
