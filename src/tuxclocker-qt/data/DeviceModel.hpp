#pragma once

#include "AssignableItem.hpp"
#include "AssignableItemData.hpp"
#include "DynamicReadableProxy.hpp"

#include <DBusTypes.hpp>
#include <Device.hpp>
#include <patterns.hpp>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QFlags>
#include <QHash>
#include <QIcon>
#include <QStandardItemModel>
#include <QPalette>
#include <Tree.hpp>

namespace TC = TuxClocker;
namespace TCDBus = TuxClocker::DBus;

// Why the fuck do I have to forward declare this?
class AssignableItem;
class AssignableProxy;

class DeviceModel : public QStandardItemModel {
public:
	DeviceModel(TC::TreeNode<TCDBus::DeviceNode> root, QObject *parent = nullptr);
	enum ColumnType {
		NameColumn = 0,	    // Node name
		InterfaceColumn = 1 // Column for presenting interfaces
	};

	enum Role {
		AssignableRole = Qt::UserRole, // Holds the data about the assignable
		AssignableProxyRole,
		ConnectionRole, // Data about the connection
		DynamicReadableProxyRole,
		InterfaceTypeRole, // InterfaceType
		NodeNameRole	   //
	};

	enum InterfaceFlag {
		Assignable = 1,
		DynamicReadable = 2,
		StaticReadable = 4,
		AllInterfaces = (Assignable | DynamicReadable | StaticReadable)
	};
	typedef QFlags<InterfaceFlag> InterfaceFlags;

	// For decoupling AssignableItems created in the model
	void applyChanges() { emit changesApplied(); }
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	static QIcon assignableIcon() { return QIcon::fromTheme("edit-entry"); }
	// Get AssignableProxy of an item
	std::optional<const AssignableProxy *> assignableProxyFromItem(QStandardItem *item);
	// AssignableProxies are associated with both row items
	// QVariant data(QModelIndex&);
	static QIcon dynamicReadableIcon() { return QIcon(":/ruler.svg"); }
	static QIcon staticReadableIcon() { return QIcon::fromTheme("help-about"); }
signals:
	void changesApplied();
private:
	Q_OBJECT

	QHash<QStandardItem *, AssignableProxy *> m_assignableProxyHash;

	// Separate handling interfaces since otherwise we run out of columns
	QStandardItem *createAssignable(
	    TC::TreeNode<TCDBus::DeviceNode> node, QDBusConnection conn, AssignableItemData data);
	std::optional<QStandardItem *> setupAssignable(
	    TC::TreeNode<TCDBus::DeviceNode> node, QDBusConnection conn);
	std::optional<QStandardItem *> setupDynReadable(
	    TC::TreeNode<TCDBus::DeviceNode> node, QDBusConnection conn);
	std::optional<QStandardItem *> setupStaticReadable(
	    TC::TreeNode<TCDBus::DeviceNode> node, QDBusConnection conn);
	QString displayText(AssignableProxy *proxy, AssignableItemData data);
	constexpr int fadeOutTime() { return 5000; } // milliseconds
	constexpr int transparency() { return 120; } // 0-255
	// Colors for items
	QColor connectionColor() { return QColor(0, 0, 255, transparency()); }	// blue
	QColor errorColor() { return QColor(255, 0, 0, transparency()); }	// red
	QColor unappliedColor() { return QColor(255, 255, 0, transparency()); } // yellow
	QColor successColor() { return QColor(0, 255, 0, transparency()); }	// green
};
