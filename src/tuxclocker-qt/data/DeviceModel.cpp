#include "DeviceModel.hpp"

#include "AssignableProxy.hpp"
#include "DynamicReadableProxy.hpp"
#include <fplus/fplus.hpp>
#include <QApplication>
#include <QDebug>
#include <QStyle>
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
					nameItem->setData(Assignable, InterfaceTypeRole);
					
					//auto style = QApplication::style();
					//auto icon = style->standardIcon(QStyle::SP_ComputerIcon);
					auto icon = assignableIcon();
					//QIcon icon("/home/jussi/Downloads/wrench.png");
					nameItem->setData(icon, Qt::DecorationRole);
					
					rowItems.append(item);
				};
			},
			pattern("org.tuxclocker.DynamicReadable") = [=, &rowItems] {
				if_let(pattern(some(arg)) = setupDynReadable(node, conn))
						= [&](auto item) {
					auto icon = dynamicReadableIcon();
					nameItem->setData(icon, Qt::DecorationRole);	
					
					nameItem->setData(DeviceModel::DynamicReadable,
						InterfaceTypeRole);
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

QStandardItem *DeviceModel::createAssignable(TC::TreeNode<TCDBus::DeviceNode> node,
		QDBusConnection conn, AssignableItemData itemData) {
	auto ifaceItem = new AssignableItem(this);
	auto proxy = new AssignableProxy(node.value().path, conn, this);
	QVariant v;
	v.setValue(itemData);
	ifaceItem->setData(v, AssignableRole);
	ifaceItem->setText("No value set");
	
	connect(ifaceItem, &AssignableItem::assignableDataChanged,
			[=](QVariant v) {
		// Only show checkbox when value has been changed
		ifaceItem->setCheckable(true);
		ifaceItem->setCheckState(Qt::Checked);
		proxy->setValue(v);
		ifaceItem->setData(unappliedColor(), Qt::BackgroundRole);
	});
	
	connect(ifaceItem, &AssignableItem::committalChanged, [=](bool on) {
		QVariant colorData = (on) ? unappliedColor() : QVariant();
		ifaceItem->setData(colorData, Qt::BackgroundRole);
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
		
		connect(anim, &QVariantAnimation::finished, [=] {
			// Set invalid color to 'reset' the color
			ifaceItem->setData(QVariant(), Qt::BackgroundRole);
		});
		
		anim->start(QAbstractAnimation::DeleteWhenStopped);
	});
	
	connect(this, &DeviceModel::changesApplied, [=] {
		// Don't apply if unchecked
		if (ifaceItem->checkState() == Qt::Checked) {
			ifaceItem->setCheckState(Qt::Unchecked);
			ifaceItem->setCheckable(false);
			// What the fuck do I need to this for?
			ifaceItem->setData(QVariant(), Qt::CheckStateRole);
			proxy->apply();
		}
	});
	return ifaceItem;
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
			TCDBus::Range r;
			d_arg >> r;
			AssignableItemData data(r.toAssignableInfo());
			return createAssignable(node, conn, data);
		}
		case QDBusArgument::ArrayType: {
			QVector<TCDBus::Enumeration> e;
			d_arg >> e;
			AssignableItemData data(toEnumVec(e));
			return createAssignable(node, conn, data);
		}
		default:
			return std::nullopt;
	}
}

template <typename T>
void updateReadItemText(QStandardItem *item, T value,
		std::optional<QString> unit) {
	// TODO: this can be made a lot (around 3x) faster by using direct copying
	// Form a string of the form "1000 MHz" if has unit
	auto text = (unit.has_value()) ?
		QString("%1 %2").arg(value).arg(unit.value()) :
		QString("%1").arg(value);
	item->setText(text);
}

std::optional<QStandardItem*> DeviceModel::setupDynReadable(
		TC::TreeNode<TCDBus::DeviceNode> node, QDBusConnection conn) {
	auto item = new QStandardItem;
	auto proxy = new DynamicReadableProxy(node.value().path, conn, this);
	auto unit = proxy->unit();
	
	connect(proxy, &DynamicReadableProxy::valueChanged, [=](ReadResult res) {
		p::match(res)(
			pattern(as<ReadableValue>(arg)) = [=](auto rv) {
				p::match(rv)(
					pattern(as<double>(arg)) = [=](auto d) {
						updateReadItemText(item, d, unit);
					},
					pattern(as<int>(arg)) = [=](auto i) {
						updateReadItemText(item, i, unit);
					},
					pattern(as<uint>(arg)) = [=](auto u) {
						updateReadItemText(item, u, unit);
					},
					pattern(_) = []{}
				);
			},
			pattern(_) = []{}
		);
	});
	
	return item;
}
