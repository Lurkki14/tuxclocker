#pragma once

//#include "AssignableItem.hpp"
#include "AssignableItemData.hpp"

#include <DBusTypes.hpp>
#include <Device.hpp>
#include <patterns.hpp>
#include <Tree.hpp>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QStandardItemModel>

namespace p = mpark::patterns;
namespace TC = TuxClocker;
namespace TCDBus = TuxClocker::DBus;


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
};
