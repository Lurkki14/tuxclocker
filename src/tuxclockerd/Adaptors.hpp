#pragma once

#include "Adaptors.hpp"
#include <DBusTypes.hpp>
#include <Device.hpp>
#include <Tree.hpp>

#include <patterns.hpp>
#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
#include <QDBusVariant>
#include <QDBusMetaType>
#include <QDebug>

using namespace TuxClocker::Device;
using namespace TuxClocker;
using namespace mpark::patterns;

namespace TCDBus = TuxClocker::DBus;

Q_DECLARE_METATYPE(TCDBus::Result<QDBusVariant>)

class DynamicReadableAdaptor : public QDBusAbstractAdaptor {
public:
	explicit DynamicReadableAdaptor(QObject *obj, DynamicReadable dr) : QDBusAbstractAdaptor(obj) {
		// Ideally this should be moved somewhere else but QMetaType does not handle namespaces well
		qDBusRegisterMetaType<TCDBus::Result<QDBusVariant>>();
		m_dynamicReadable = dr;
	}
public Q_SLOTS:
	TCDBus::Result<QDBusVariant> value() {
		QVariant v;
		TCDBus::Result<QDBusVariant> res{.error = false};
		/* We have to unwrap the value here, another option would be to convert the std::variant
		 * to char*, but that comes with its own issues */
		match(m_dynamicReadable.read())
			(pattern(as<ReadableValue>(arg)) = [&](auto val) {
				match(val)
					(pattern(as<uint>(arg)) = [&](auto u) {
						v.setValue(u);
					},
					pattern(as<double>(arg)) = [&](auto d) {
						v.setValue(d);
					});
			},
			pattern(as<ReadError>(arg)) = [&](auto err) {
				v.setValue(static_cast<int>(err));
				res.error = true;
		});
		res.value = QDBusVariant(v);
		return res;
	}
private:
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.tuxclocker.DynamicReadable")

	DynamicReadable m_dynamicReadable;
};

Q_DECLARE_METATYPE(TCDBus::Range)
Q_DECLARE_METATYPE(TCDBus::Enumeration)
Q_DECLARE_METATYPE(TCDBus::Result<int>)

class AssignableAdaptor : public QDBusAbstractAdaptor {
public:
	explicit AssignableAdaptor(QObject *obj, Assignable a, DeviceNode devNode) :
			QDBusAbstractAdaptor(obj), m_assignable(a), m_devNode(devNode) {
		qDBusRegisterMetaType<TCDBus::Range>();
		qDBusRegisterMetaType<TCDBus::Enumeration>();
		qDBusRegisterMetaType<QVector<TCDBus::Enumeration>>();
		qDBusRegisterMetaType<TCDBus::Result<int>>();
		QVariant a_info;
		// Unwrap AssignableInfo :(
		match(a.assignableInfo())
			(pattern(as<RangeInfo>(arg)) = [&](auto r_info) {
				match(r_info)
					(pattern(as<Range<double>>(arg)) = [&](auto dr) {
						TCDBus::Range r{
							.min = QDBusVariant(QVariant(dr.min)),
							.max = QDBusVariant(QVariant(dr.max))
						};
						a_info.setValue(r);
					},
					pattern(as<Range<int>>(arg)) = [&](auto ir) {
						TCDBus::Range r{
							.min = QDBusVariant(QVariant(ir.min)),
							.max = QDBusVariant(QVariant(ir.max))
						};
						a_info.setValue(r);
				});
			},
			pattern(as<EnumerationVec>(arg)) = [&](auto enums) {
				QVector<TCDBus::Enumeration> entries;
				for (const auto &e : enums)
					entries.append({e.key, QString::fromStdString(e.name)});
				a_info.setValue(entries);
		});
			
		m_dbusAssignableInfo = QDBusVariant(a_info);
	}
	QDBusVariant assignableInfo_() {return m_dbusAssignableInfo;}
	QString hash_() {return QString::fromStdString(m_devNode.hash);}
public Q_SLOTS:
	TCDBus::Result<int> assign(QDBusVariant arg_) {
		auto v = arg_.variant();
		
		std::optional<AssignmentError> retval;
		
		match(m_assignable.assignableInfo())
			(pattern(as<RangeInfo>(arg)) = [&](auto ri) {
				match(ri)
					(pattern(as<Range<double>>(_)) = [&] {
						retval = m_assignable.assign(v.value<double>());
					},
					pattern(as<Range<int>>(_)) = [&] {
						retval = m_assignable.assign(v.value<int>());
				});
			},
			pattern(as<EnumerationVec>(_)) = [&] {
				retval = m_assignable.assign(v.value<uint>());
		});
		
		TCDBus::Result<int> res{.error = false, 0};
		// Check if optional contains error
		if (retval.has_value()) {
			res.error = true;
			res.value = static_cast<int>(retval.value());
		}
		
		return res;
	}
private:
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.tuxclocker.Assignable")
	Q_PROPERTY(QDBusVariant assignableInfo READ assignableInfo_)
	Q_PROPERTY(QString hash READ hash_)
	
	Assignable m_assignable;
	DeviceNode m_devNode;
	QDBusVariant m_dbusAssignableInfo;
};

Q_DECLARE_METATYPE(TCDBus::DeviceNode)
Q_DECLARE_METATYPE(TCDBus::FlatTreeNode<TCDBus::DeviceNode>)

// Holds the main tree and returns it as a list (because parsing XML sucks)
class MainAdaptor : public QDBusAbstractAdaptor {
public:
	explicit MainAdaptor(QObject *obj, TreeNode<TCDBus::DeviceNode> node) : QDBusAbstractAdaptor(obj) {
		qDBusRegisterMetaType<TCDBus::DeviceNode>();
		qDBusRegisterMetaType<TCDBus::FlatTreeNode<TCDBus::DeviceNode>>();
		qDBusRegisterMetaType<QVector<TCDBus::FlatTreeNode<TCDBus::DeviceNode>>>();
		
		for (const auto &f_node : node.toFlatTree().nodes) {
			// Copy child indices
			QVector<int> childIndices;
			for (const auto &i : f_node.childIndices)
				childIndices.append(i);
				
			TCDBus::FlatTreeNode<TCDBus::DeviceNode> fn{f_node.value, childIndices};
			m_flatTree.append(fn);
		}
	}
public Q_SLOTS:
	QVector<TCDBus::FlatTreeNode<TCDBus::DeviceNode>> flatDeviceTree() {return m_flatTree;}
private:
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.tuxclocker")
	
	TreeNode<TCDBus::DeviceNode> m_rootNode;
	QVector<TCDBus::FlatTreeNode<TCDBus::DeviceNode>> m_flatTree;
};
