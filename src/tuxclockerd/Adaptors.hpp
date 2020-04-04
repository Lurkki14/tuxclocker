#pragma once

#include "Adaptors.hpp"
#include <Device.hpp>

#include <QDBusAbstractAdaptor>
#include <QDBusVariant>

using namespace TuxClocker::Device;

Q_DECLARE_METATYPE(ReadResult)

class DynamicReadableAdaptor : public QDBusAbstractAdaptor {
public:
		explicit DynamicReadableAdaptor(QObject *obj, DynamicReadable dr) : QDBusAbstractAdaptor(obj) {
			m_dynamicReadable = dr;
		}
public Q_SLOTS:
		QDBusVariant value() {
			QVariant v;
			v.setValue(m_dynamicReadable.read());
			return QDBusVariant(v);
		}
private:
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.tuxclocker.DynamicReadable")
	
	DynamicReadable m_dynamicReadable;
};
