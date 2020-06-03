#include "DeviceModel.hpp"

#include "AssignableProxy.hpp"
#include "DynamicReadableProxy.hpp"
#include <fplus/fplus.hpp>
#include <QDebug>
#include <QVariantAnimation>

// 'match' is a method in QAbstractItemModel :(
namespace p = mpark::patterns;
using namespace fplus;
using namespace mpark::patterns;
using namespace TuxClocker::Device;

Q_DECLARE_METATYPE(AssignableItemData)
Q_DECLARE_METATYPE(TCDBus::Enumeration)
Q_DECLARE_METATYPE(TCDBus::Range)
Q_DECLARE_METATYPE(EnumerationVec)

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
	
	std::function<void(TC::TreeNode<TCDBus::DeviceNode> node,
		QStandardItem*)> traverse;
	traverse = [&traverse, this](auto node, auto item) {
		auto conn = QDBusConnection::systemBus();
		QDBusInterface nodeIface("org.tuxclocker",
			node.value().path,"org.tuxclocker.Node", conn);
		auto nodeName = nodeIface.property("name").toString();
		
		QList<QStandardItem*> rowItems;
		auto nameItem = new QStandardItem;
		nameItem->setText(nodeName);
		rowItems.append(nameItem);
		
		p::match(node.value().interface) (
			pattern("org.tuxclocker.Assignable") = [=, &rowItems]{
				if_let(pattern(some(arg)) = setupAssignable(node, conn))
						= [&](auto item) {
					rowItems.append(item);
				};
			},
			pattern("org.tuxclocker.DynamicReadable") = [=, &rowItems] {
				if_let(pattern(some(arg)) = setupDynReadable(node, conn))
						= [&](auto item) {
					rowItems.append(item);
				};
			},
			pattern(_) = []{}
		);
		item->appendRow(rowItems);
		
		for (auto c_node : node.children())
			traverse(c_node, nameItem);
	};
	auto rootItem = invisibleRootItem();
	
	for (auto &node : root.children())
		traverse(node, rootItem);
}

EnumerationVec toEnumVec(QVector<TCDBus::Enumeration> enums) {
	return transform([](auto e) {
		return Enumeration{e.name.toStdString(), e.key};
	}, enums.toStdVector());
}

void DeviceModel::connectAssignable(TC::TreeNode<TCDBus::DeviceNode> node,
		QDBusConnection conn, AssignableItem *ifaceItem) {
	auto proxy = new AssignableProxy(node.value().path, conn, this);
	connect(ifaceItem, &AssignableItem::assignableDataChanged,
			[=](QVariant v) {
		proxy->setValue(v);
		ifaceItem->setData(unappliedColor(), Qt::BackgroundRole);
	});
	
	connect(proxy, &AssignableProxy::applied, [=](auto err) {
		// Fade out result color
		auto startColor = (err.has_value()) ? errorColor()
			: successColor();
		auto anim = new QVariantAnimation;
		anim->setDuration(fadeOutTime());
		anim->setStartValue(startColor);
		anim->setEndValue(QPalette().color(QPalette::Base));
		
		connect(anim, &QVariantAnimation::valueChanged, [=](QVariant v) {
			QVariant iv;
			iv.setValue(v.value<QColor>());
			ifaceItem->setData(iv, Qt::BackgroundRole);
		});
		anim->start(QAbstractAnimation::DeleteWhenStopped);
	});
	
	connect(this, &DeviceModel::changesApplied, [=] {
		proxy->apply();
	});
}

std::optional<QStandardItem*> DeviceModel::setupAssignable(
		TC::TreeNode<TCDBus::DeviceNode> node, QDBusConnection conn) {
	QDBusInterface ifaceNode("org.tuxclocker", node.value().path,
		"org.tuxclocker.Assignable", conn);
	// Should never fail
	auto a_info =
		qvariant_cast<QDBusVariant>(ifaceNode.property("assignableInfo"))
		.variant();
	/* TODO: bad hack: this code can only differentiate between
		arrays and structs: make it based on signature instead */
	auto d_arg = qvariant_cast<QDBusArgument>(a_info);
	switch (d_arg.currentType()) {
		case QDBusArgument::StructureType: {
			auto ifaceItem = new AssignableItem;
			ifaceItem->setEditable(true);
			//ifaceItem->setCheckable(true);
			/*auto proxy = new AssignableProxy(node.value().path, conn, this);
			connect(ifaceItem, &AssignableItem::assignableDataChanged,
					[=](QVariant v) {
				proxy->setValue(v);
				ifaceItem->setData(unappliedColor(), Qt::BackgroundRole);
			});
			
			connect(proxy, &AssignableProxy::applied, [=](auto err) {
				// Fade out result color
				auto startColor = (err.has_value()) ? errorColor()
					: successColor();
				auto anim = new QVariantAnimation;
				anim->setDuration(fadeOutTime());
				anim->setStartValue(startColor);
				anim->setEndValue(QPalette().color(QPalette::Base));
				
				connect(anim, &QVariantAnimation::valueChanged, [=](QVariant v) {
					QVariant iv;
					iv.setValue(v.value<QColor>());
					ifaceItem->setData(iv, Qt::BackgroundRole);
				});
				anim->start(QAbstractAnimation::DeleteWhenStopped);
			});
			
			connect(this, &DeviceModel::changesApplied, [=] {
				proxy->apply();
			});*/
			
			connectAssignable(node, conn, ifaceItem);

			TCDBus::Range r;
			d_arg >> r;
			QVariant v;
			AssignableItemData data(r.toAssignableInfo());
			v.setValue(data);
			ifaceItem->setData(v, Role::AssignableRole);
			return ifaceItem;
		}
		case QDBusArgument::ArrayType: {
			auto ifaceItem = new AssignableItem;
			
			connectAssignable(node, conn, ifaceItem);
			
			QVector<TCDBus::Enumeration> e;
			// TODO: convert to EnumerationVec
			d_arg >> e;
			QVariant v;
			AssignableItemData data(toEnumVec(e));
			v.setValue(data);
			ifaceItem->setData(v, Role::AssignableRole);
			ifaceItem->setText(e.first().name);
			return ifaceItem;
		}
		default:
			return std::nullopt;
	}
}

std::optional<QStandardItem*> DeviceModel::setupDynReadable(
		TC::TreeNode<TCDBus::DeviceNode> node, QDBusConnection conn) {
	auto item = new QStandardItem;
	auto proxy = new DynamicReadableProxy(node.value().path, conn, this);
	
	connect(proxy, &DynamicReadableProxy::valueChanged, [=](ReadResult res) {
		p::match(res)(
			pattern(as<ReadableValue>(arg)) = [=](auto rv) {
				p::match(rv)(
					pattern(as<double>(arg)) = [=](auto d) {
						item->setText(QString::number(d));
					},
					pattern(as<int>(arg)) = [=](auto i) {
						item->setText(QString::number(i));
					},
					pattern(as<uint>(arg)) = [=](auto u) {
						item->setText(QString::number(u));
					},
					pattern(_) = []{}
				);
			},
			pattern(_) = []{}
		);
	});
	
	return item;
}
