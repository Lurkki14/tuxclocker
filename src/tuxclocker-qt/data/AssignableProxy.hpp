#pragma once

// Acts as a buffer for unapplied assignables and applies them asynchronously

#include <Device.hpp>
#include <memory>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QObject>
#include <variant>

namespace TC = TuxClocker;

class AssignableProxy : public QObject {
public:
	AssignableProxy(QString path, QDBusConnection conn,
		QObject *parent = nullptr);
	void apply();
	void setValue(QVariant v) {m_value = v;}
signals:
	void applied(std::optional<TC::Device::AssignmentError>);
private:
	Q_OBJECT
	
	QVariant m_value;
	QDBusInterface *m_iface;
};
