#pragma once

#include <Device.hpp>
#include <optional>
#include <QDBusArgument>
#include <QDBusVariant>
#include <QDBusMetaType>
#include <QVector>

namespace TC = TuxClocker;

namespace TuxClocker::DBus {

template <typename T> struct Result {
	bool error;
	T value;

	friend QDBusArgument &operator<<(QDBusArgument &arg, const Result<T> r) {
		arg.beginStructure();
		arg << r.error << r.value;
		arg.endStructure();
		return arg;
	}

	friend const QDBusArgument &operator>>(const QDBusArgument &arg, Result<T> &r) {
		arg.beginStructure();
		arg >> r.error >> r.value;
		arg.endStructure();
		return arg;
	}
	std::optional<T> toOptional() {
		std::optional<T> r = (error) ? std::optional(value) : std::nullopt;
		return r;
	}
};

struct Range {
	QDBusVariant min, max;
	friend QDBusArgument &operator<<(QDBusArgument &arg, const Range r) {
		arg.beginStructure();
		arg << r.min << r.max;
		arg.endStructure();
		return arg;
	}
	friend const QDBusArgument &operator>>(const QDBusArgument &arg, Range &r) {
		arg.beginStructure();
		arg >> r.min >> r.max;
		arg.endStructure();
		return arg;
	}
	TC::Device::AssignableInfo toAssignableInfo() {
		auto min_v = min.variant();
		auto max_v = max.variant();

		auto im = static_cast<QVariant::Type>(QMetaType::Int);
		auto dm = static_cast<QVariant::Type>(QMetaType::Double);
		if (min_v.type() == im && max_v.type() == im)
			return TC::Device::Range<int>(min_v.value<int>(), max_v.value<int>());
		if (min_v.type() == dm && max_v.type() == dm)
			return TC::Device::Range<double>(
			    min_v.value<double>(), max_v.value<double>());
		// Should never reach here
		return TC::Device::Range<int>(0, 0);
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
	friend const QDBusArgument &operator>>(const QDBusArgument &arg, Enumeration &e) {
		arg.beginStructure();
		arg >> e.key >> e.name;
		arg.endStructure();
		return arg;
	}
};

/* MOC crap doubles this somewhere
TC::Device::AssignableInfo enumVecToAssignableInfo(QVector<Enumeration> enums) {
	TC::Device::EnumerationVec v;
	for (const auto &e : enums)
		v.push_back(TC::Device::Enumeration(e.name.toStdString(), e.key));
	return v;
}
*/

struct DeviceNode {
	QString interface;
	QString path;
	friend QDBusArgument &operator<<(QDBusArgument &arg, const DeviceNode d) {
		arg.beginStructure();
		arg << d.interface << d.path;
		arg.endStructure();
		return arg;
	}
	friend const QDBusArgument &operator>>(const QDBusArgument &arg, DeviceNode &d) {
		arg.beginStructure();
		arg >> d.interface >> d.path;
		arg.endStructure();
		return arg;
	}
};

template <typename T> struct FlatTreeNode {
	T value;
	QVector<int> childIndices;
	friend QDBusArgument &operator<<(QDBusArgument &arg, const FlatTreeNode<T> f) {
		arg.beginStructure();
		arg << f.value << f.childIndices;
		arg.endStructure();
		return arg;
	}
	friend const QDBusArgument &operator>>(const QDBusArgument &arg, FlatTreeNode<T> &f) {
		arg.beginStructure();
		arg >> f.value >> f.childIndices;
		arg.endStructure();
		return arg;
	}
};

}; // namespace TuxClocker::DBus
