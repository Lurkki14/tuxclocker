#pragma once

#include <QDBusArgument>
#include <QDBusVariant>
#include <QDBusMetaType>
	
namespace TuxClocker::DBus {

struct ReadResult {
	bool error;
	QDBusVariant value;
	
	friend QDBusArgument &operator<<(QDBusArgument &arg, const ReadResult r) {
		arg.beginStructure();
		arg << r.error << r.value;
		arg.endStructure();
		return arg;
	}
		
	friend const QDBusArgument &operator>>(const QDBusArgument &arg, ReadResult r) {
		arg.beginStructure();
		arg >> r.error >> r.value;
		arg.endStructure();
		return arg;
	}
};

};

//Q_DECLARE_METATYPE(TuxClocker::DBus::ReadResult)
