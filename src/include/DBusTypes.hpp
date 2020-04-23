#pragma once

#include <QDBusArgument>
#include <QDBusVariant>
#include <QDBusMetaType>
#include <QVector>
	
namespace TuxClocker::DBus {

template <typename T>
struct Result {
	bool error;
	T value;
	
	friend QDBusArgument &operator<<(QDBusArgument &arg, const Result<T> r) {
		arg.beginStructure();
		arg << r.error << r.value;
		arg.endStructure();
		return arg;
	}
		
	friend const QDBusArgument &operator>>(const QDBusArgument &arg, Result<T> r) {
		arg.beginStructure();
		arg >> r.error >> r.value;
		arg.endStructure();
		return arg;
	}
};

struct Range {
	QDBusVariant min, max;
	friend QDBusArgument &operator<<(QDBusArgument &arg, const Range r) {
		arg.beginStructure();
		arg << r.min<< r.max;
		arg.endStructure();
		return arg;
	}
	friend const QDBusArgument &operator>>(const QDBusArgument &arg, Range r) {
		arg.beginStructure();
		arg >> r.min >> r.max;
		arg.endStructure();
		return arg;
	}
};

struct Enumeration {
	uint key;
	QString name;
	friend QDBusArgument &operator<<(QDBusArgument &arg, const Enumeration e) {
		arg.beginStructure();
		arg << e.key << e.name;
		arg.endStructure();
		return arg;
	}
	friend const QDBusArgument &operator>>(const QDBusArgument &arg, Enumeration e) {
		arg.beginStructure();
		arg >> e.key >> e.name;
		arg.endStructure();
		return arg;
	}	
};

struct DeviceNode {
	QString interface;
	QString path;
	friend QDBusArgument &operator<<(QDBusArgument &arg, const DeviceNode d) {
		arg.beginStructure();
		arg << d.interface << d.path;
		arg.endStructure();
		return arg;
	}
	friend const QDBusArgument &operator>>(const QDBusArgument &arg, DeviceNode d) {
		arg.beginStructure();
		arg >> d.interface >> d.path;
		arg.endStructure();
		return arg;
	}
};

template <typename T>
struct FlatTreeNode {
	T value;
	QVector<int> childIndices;
	friend QDBusArgument &operator<<(QDBusArgument &arg, const FlatTreeNode<T> f) {
		arg.beginStructure();
		arg << f.value << f.childIndices;
		arg.endStructure();
		return arg;
	}
	friend const QDBusArgument &operator>>(const QDBusArgument &arg, FlatTreeNode<T> f) {
		arg.beginStructure();
		arg >> f.value >> f.childIndices;
		arg.endStructure();
		return arg;
	}
};

};

//Q_DECLARE_METATYPE(TuxClocker::DBus::ReadResult)
