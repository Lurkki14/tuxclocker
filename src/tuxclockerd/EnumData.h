#pragma once

#include <QDBusMetaType>
#include <QString>

struct EnumData {
	QString name;
	unsigned value;

	friend QDBusArgument &operator<<(QDBusArgument &arg, const EnumData &data) {
		arg.beginStructure();
		arg << data.name << data.value;
		arg.endStructure();
		return arg;
	}
	friend const QDBusArgument &operator>>(const QDBusArgument &arg, EnumData &data) {
		arg.beginStructure();
		arg >> data.name >> data.value;
		arg.endStructure();
		return arg;
	}
	/*friend  QDBusArgument &operator<<(QDBusArgument &arg, const QList <EnumData> list) {
		arg.beginArray();

		for (auto data : list) {
			arg << data;
		}
		arg.endArray();
		return arg;
	}

	friend const QDBusArgument &operator>>(const QDBusArgument &arg, QList <EnumData> &list) {
		arg.beginArray();
		list.clear();

		while (!arg.atEnd()) {
			EnumData data;
			arg >> data;
			list.append(data);
		}
		arg.endArray();
		return arg;
	}*/
};

Q_DECLARE_METATYPE(EnumData)
