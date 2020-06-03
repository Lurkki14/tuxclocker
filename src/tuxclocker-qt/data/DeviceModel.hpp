#pragma once

#include "AssignableItem.hpp"
#include "AssignableItemData.hpp"

#include <DBusTypes.hpp>
#include <Device.hpp>
#include <patterns.hpp>
#include <Tree.hpp>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QStandardItemModel>
#include <QPalette>

namespace TC = TuxClocker;
namespace TCDBus = TuxClocker::DBus;

// Why the fuck do I have to forward declare this?
class AssignableItem;

class DeviceModel : public QStandardItemModel {
public:
	DeviceModel(TC::TreeNode<TCDBus::DeviceNode> root, QObject *parent = nullptr);
	enum ColumnType {
		Name = 0, // Node name
		Interface = 1 // Column for presenting interfaces
	};
	
	enum Role {
		AssignableRole = Qt::UserRole, // Holds the data about the assignable
		ConnectionRole // Data about the connection
	};
	
	enum class FilterFlag {
		
	};
	// For decoupling AssignableItems created in the model
	void applyChanges() {emit changesApplied();}
signals:
	void changesApplied();
private:
	Q_OBJECT
	
	// Separate handling interfaces since otherwise we run out of columns
	void connectAssignable(TC::TreeNode<TCDBus::DeviceNode> node,
		QDBusConnection conn, AssignableItem *ifaceItem);
	std::optional<QStandardItem*> setupAssignable(
		TC::TreeNode<TCDBus::DeviceNode> node, QDBusConnection conn);
	std::optional<QStandardItem*> setupDynReadable(
		TC::TreeNode<TCDBus::DeviceNode> node, QDBusConnection conn);
	constexpr int fadeOutTime() {return 5000;} // milliseconds
	constexpr int transparency() {return 120;} // 0-255
	// Colors for items
	QColor errorColor() {return QColor(255, 0, 0, transparency());} // red
	QColor unappliedColor() {return QColor(255, 255, 0, transparency());} // yellow
	QColor successColor() {return QColor(0, 255, 0, transparency());} // green
};
