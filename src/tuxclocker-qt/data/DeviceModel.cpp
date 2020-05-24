#include "DeviceModel.hpp"

Q_DECLARE_METATYPE(AssignableItemData)
Q_DECLARE_METATYPE(TCDBus::Enumeration)
Q_DECLARE_METATYPE(TCDBus::Range)

DeviceModel::DeviceModel(TC::TreeNode<TCDBus::DeviceNode> root, QObject *parent) :
		QStandardItemModel(parent) {
	qDBusRegisterMetaType<TCDBus::Enumeration>();
	qDBusRegisterMetaType<QVector<TCDBus::Enumeration>>();
	qDBusRegisterMetaType<TCDBus::Range>();
	/* Data storage:
		- Interface column should store assignable info for editors
		- Name colums should store the interface type for filtering 
		- Parametrization/connection data, where? */ 
			
	setColumnCount(2);
	
	std::function<void(TC::TreeNode<TCDBus::DeviceNode> node, QStandardItem*)> traverse;
	traverse = [&traverse](auto node, auto item) {
		auto conn = QDBusConnection::systemBus();
		QDBusInterface nodeIface("org.tuxclocker", node.value().path, "org.tuxclocker.Node", conn);
		auto nodeName = nodeIface.property("name").toString();
		
		QList<QStandardItem*> rowItems;
		auto nameItem = new QStandardItem;
		nameItem->setText(nodeName);
		rowItems.append(nameItem);
		
		// Add assignable data if this is one
		if (node.value().interface == "org.tuxclocker.Assignable") {
			QDBusInterface ifaceNode("org.tuxclocker", node.value().path,
				"org.tuxclocker.Assignable", conn);
			// Should never fail
			auto a_info =
				qvariant_cast<QDBusVariant>(ifaceNode.property("assignableInfo"))
				.variant();
			/* TODO: bad hack: this code can only differentiate between
				arrays and structs */
			auto d_arg = qvariant_cast<QDBusArgument>(a_info);
			switch (d_arg.currentType()) {
				case QDBusArgument::StructureType: {
					auto ifaceItem = new QStandardItem;
					TCDBus::Range r;
					d_arg >> r;
					QVariant v;
					AssignableItemData data(r.toAssignableInfo());
					v.setValue(data);
					ifaceItem->setData(v, Role::AssignableRole);
					ifaceItem->setText(r.min.variant().toString());
					rowItems.append(ifaceItem);
					break;
				}
				case QDBusArgument::ArrayType: {
					auto ifaceItem = new QStandardItem;
					QVector<TCDBus::Enumeration> e;
					d_arg >> e;
					QVariant v;
					v.setValue(e);
					ifaceItem->setData(v, Role::AssignableRole);
					ifaceItem->setText(e.first().name);
					rowItems.append(ifaceItem);
					break;
				}
				default:
					break;
			}
		}
		item->appendRow(rowItems);
		
		for (auto c_node : node.children())
			traverse(c_node, nameItem);
	};
	auto rootItem = invisibleRootItem();
	
	for (auto &node : root.children())
		traverse(node, rootItem);
}
