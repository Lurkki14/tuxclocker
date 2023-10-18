#include "DeviceModel.hpp"

#include "AssignableProxy.hpp"
#include "DynamicReadableProxy.hpp"
#include <fplus/fplus.hpp>
#include <Globals.hpp>
#include <libintl.h>
#include <Utils.hpp>
#include <QApplication>
#include <QDBusReply>
#include <QDebug>
#include <QtGlobal>
#include <QStyle>
#include <QVariantAnimation>

#define _(String) gettext(String)

// 'match' is a method in QAbstractItemModel :(
namespace p = mpark::patterns;
using namespace fplus;
using namespace mpark::patterns;
using namespace TuxClocker::Device;

Q_DECLARE_METATYPE(AssignableItemData)
Q_DECLARE_METATYPE(AssignableProxy *)
Q_DECLARE_METATYPE(DynamicReadableProxy *)
Q_DECLARE_METATYPE(TCDBus::Enumeration)
Q_DECLARE_METATYPE(TCDBus::Range)
Q_DECLARE_METATYPE(EnumerationVec)
Q_DECLARE_METATYPE(TCDBus::Result<QString>)

DeviceModel::DeviceModel(TC::TreeNode<TCDBus::DeviceNode> root, QObject *parent)
    : QStandardItemModel(parent) {
	qDBusRegisterMetaType<TCDBus::Enumeration>();
	qDBusRegisterMetaType<QVector<TCDBus::Enumeration>>();
	qDBusRegisterMetaType<TCDBus::Range>();
	qDBusRegisterMetaType<TCDBus::Result<QString>>();

	/* Data storage:
		- Interface column should store assignable info for editors
		- Name colums should store the interface type for filtering
		- Parametrization/connection data, where? */

	setColumnCount(2);

	std::function<void(TC::TreeNode<TCDBus::DeviceNode> node, QStandardItem *)> traverse;
	traverse = [&traverse, this](auto node, auto item) {
		auto conn = QDBusConnection::systemBus();
		QDBusInterface nodeIface(
		    "org.tuxclocker", node.value().path, "org.tuxclocker.Node", conn);
		auto nodeName = nodeIface.property("name").toString();

		QList<QStandardItem *> rowItems;
		auto nameItem = new QStandardItem;
		nameItem->setText(nodeName);
		rowItems.append(nameItem);

		p::match(node.value().interface)(
		    pattern("org.tuxclocker.Assignable") =
			[=, &rowItems] {
				if_let(pattern(some(arg)) =
					   setupAssignable(node, conn)) = [&](auto item) {
					nameItem->setData(Assignable, InterfaceTypeRole);
					auto icon = assignableIcon();
					nameItem->setData(icon, Qt::DecorationRole);
					rowItems.append(item);
				};
			},
		    pattern("org.tuxclocker.DynamicReadable") =
			[=, &rowItems] {
				if_let(pattern(some(arg)) =
					   setupDynReadable(node, conn)) = [&](auto item) {
					auto icon = dynamicReadableIcon();
					nameItem->setData(icon, Qt::DecorationRole);

					nameItem->setData(
					    DeviceModel::DynamicReadable, InterfaceTypeRole);
					rowItems.append(item);
					// qDebug() << item->data(DynamicReadableProxyRole);
				};
			},
		    pattern("org.tuxclocker.StaticReadable") =
			[=, &rowItems] {
				if_let(pattern(some(arg)) =
					   setupStaticReadable(node, conn)) = [&](auto item) {
					auto icon = staticReadableIcon();
					nameItem->setData(icon, Qt::DecorationRole);
					nameItem->setData(
					    DeviceModel::StaticReadable, InterfaceTypeRole);
					rowItems.append(item);
				};
			},
		    pattern(_) = [] {});
		item->appendRow(rowItems);

		for (auto c_node : node.children())
			traverse(c_node, nameItem);
	};
	auto rootItem = invisibleRootItem();

	for (auto &node : root.children())
		traverse(node, rootItem);
}

EnumerationVec toEnumVec(QVector<TCDBus::Enumeration> enums) {
	std::vector<TCDBus::Enumeration> stdEnumVec(enums.begin(), enums.end());
	return transform(
	    [](auto e) {
		    return Enumeration{e.name.toStdString(), e.key};
	    },
	    stdEnumVec);
}

std::optional<const AssignableProxy *> DeviceModel::assignableProxyFromItem(QStandardItem *item) {
	return (m_assignableProxyHash.contains(item))
		   ? std::optional(m_assignableProxyHash.value(item))
		   : std::nullopt;
}

QString fromAssignmentArgument(AssignmentArgument a_arg) {
	return p::match(a_arg)(
	    pattern(as<int>(arg)) = [](auto i) { return QString::number(i); },
	    pattern(as<uint>(arg)) = [](auto u) { return QString::number(u); },
	    pattern(as<double>(arg)) = [](auto d) { return QString::number(d); },
	    pattern(_) = [] { return QString(""); });
}

QStandardItem *DeviceModel::createAssignable(
    TC::TreeNode<TCDBus::DeviceNode> node, QDBusConnection conn, AssignableItemData itemData) {
	auto ifaceItem = new AssignableItem(itemData.unit(), this);
	auto proxy = new AssignableProxy(node.value().path, conn, this);

	connect(proxy, &AssignableProxy::connectionValueChanged, [=](auto result, auto text) {
		p::match(result)(
		    pattern(as<QVariant>(arg)) =
			[=](auto v) {
				QVariant data;
				data.setValue(connectionColor());

				if (!ifaceItem->committal()) {
					// Don't set color when committed
					ifaceItem->clearTargetText();
					ifaceItem->setData(data, Qt::BackgroundRole);
				}
				// TODO: small annoyance: causes 10 W -> 11 W
				// instead of desired 10 -> 11 W
				// when committing
				ifaceItem->setCurrentValueText(text);
			},
		    pattern(_) = [] {});
	});

	connect(proxy, &AssignableProxy::connectionStopped, [=](auto value) {
		m_activeConnections.removeOne(qvariant_cast<DynamicReadableConnectionData>(value));
	});

	connect(proxy, &AssignableProxy::connectionSucceeded, [=](auto value) {
		m_activeConnections.append(qvariant_cast<DynamicReadableConnectionData>(value));
		//  Write successful connection value to settings
		auto assSetting = AssignableSetting{
		    .assignablePath = proxy->dbusPath(),
		    .value = value,
		};
		auto newSettings =
		    Settings::setAssignableSetting(Globals::g_settingsData, assSetting);
		Utils::writeAssignableSetting(newSettings, assSetting);
	});

	QVariant pv;
	pv.setValue(proxy);
	ifaceItem->setData(pv, AssignableProxyRole);
	QVariant v;
	v.setValue(itemData);
	ifaceItem->setData(v, AssignableRole);

	// Set initial text to current value (one-time at startup)
	// Related to TODO above
	ifaceItem->setCurrentValueText(displayText(proxy, itemData));

	connect(ifaceItem, &AssignableItem::assignableDataChanged, [=](QVariant v) {
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
		ifaceItem->applyTargetText();

		bool success = !err.has_value();

		if (success) {
			// Write successfully changed value to settings
			// TODO: parametrization won't be saved here
			Utils::writeAssignableSetting(
			    Globals::g_settingsData, AssignableSetting{
							 .assignablePath = proxy->dbusPath(),
							 .value = proxy->targetValue(),
						     });
		}

		// Fade out result color
		auto startColor = !success ? errorColor() : successColor();
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
			proxy->apply();
		} else {
			ifaceItem->clearTargetText();
		}
		// Make unchecked item uncheckable too
		// TODO: do this as soon as item is unchecked, haven't found a good way so far
		ifaceItem->setCheckState(Qt::Unchecked);
		ifaceItem->setCheckable(false);
		// What the fuck do I need to this for?
		ifaceItem->setData(QVariant(), Qt::CheckStateRole);
	});
	return ifaceItem;
}

QString DeviceModel::displayText(AssignableProxy *proxy, AssignableItemData data) {
	auto currentValue = proxy->currentValue();
	auto defVal = _("No value set");
	if (!currentValue.has_value())
		return defVal;

	auto a_info = data.assignableInfo();
	// Try to get text representation of current enum
	if (std::holds_alternative<EnumerationVec>(a_info) &&
	    std::holds_alternative<uint>(currentValue.value())) {
		auto enumVec = std::get<EnumerationVec>(a_info);
		auto index = std::get<uint>(currentValue.value());
		// Find the key
		for (auto &e : enumVec) {
			if (index == e.key) {
				return QString::fromStdString(e.name);
			}
		}
		// TODO: not that helpful message without any name or DBus path
		qWarning("Tried to get name for invalid Enumeration index %u", index);
		return defVal;
	}

	auto unit = data.unit();
	if (std::holds_alternative<RangeInfo>(a_info)) {
		return fromAssignmentArgument(currentValue.value());
	}
	return defVal;
}

QVariant DeviceModel::data(const QModelIndex &index, int role) const {
	if (index.row() != DeviceModel::NameColumn && role == DeviceModel::NodeNameRole) {
		// Get name from adjacent column
		auto nameIndex = this->index(index.row(), DeviceModel::NameColumn, index.parent());
		return nameIndex.data(Qt::DisplayRole);
	}
	if (index.column() != InterfaceColumn && role == DynamicReadableProxyRole) {
		auto idx = this->index(index.row(), DeviceModel::InterfaceColumn, index.parent());
		return idx.data(DynamicReadableProxyRole);
	}
	return QStandardItemModel::data(index, role);
}

std::optional<QString> fromDBusResult(TCDBus::Result<QString> res) {
	if (res.error)
		return std::nullopt;
	return res.value;
}

std::optional<QStandardItem *> DeviceModel::setupAssignable(
    TC::TreeNode<TCDBus::DeviceNode> node, QDBusConnection conn) {
	QDBusInterface ifaceNode(
	    "org.tuxclocker", node.value().path, "org.tuxclocker.Assignable", conn);

	// Calling QObject::property used to work for getting (some) DBus properties but now we have
	// to do this..
	QDBusInterface propIface{
	    "org.tuxclocker", node.value().path, "org.freedesktop.DBus.Properties", conn};
	QDBusReply<QDBusVariant> a_reply =
	    propIface.call("Get", "org.tuxclocker.Assignable", "assignableInfo");
	QDBusReply<QDBusVariant> u_reply =
	    propIface.call("Get", "org.tuxclocker.Assignable", "unit");

	if (!a_reply.isValid() || !u_reply.isValid()) {
		// This code path shouldn't be reached
		// The DBus path is contained in the error message
		qWarning("Could not get assignableInfo or unit for Assignable "
			 "due to error(s) \"%s\" and \"%s\"",
		    qPrintable(a_reply.error().message()), qPrintable(u_reply.error().message()));
		return std::nullopt;
	}
	// Calling QDBusReply::value() should ge safe here
	// Need to serialize manually into the proper types
	auto unit = fromDBusResult(
	    qdbus_cast<TCDBus::Result<QString>>(u_reply.value().variant().value<QDBusArgument>()));

	// What a mess...
	auto assInfoRaw =
	    a_reply.value().variant().value<QDBusVariant>().variant().value<QDBusArgument>();
	// TODO: get initial value

	// Use DBus signature to know the type of Assignable
	if (assInfoRaw.currentSignature() == "(vv)") {
		// RangeInfo
		auto range = qdbus_cast<TCDBus::Range>(assInfoRaw);
		AssignableItemData data{range.toAssignableInfo(), unit};
		return createAssignable(node, conn, data);
	} else if (assInfoRaw.currentSignature() == "a(us)") {
		// EnumerationVec
		auto enumVec = qdbus_cast<QVector<TCDBus::Enumeration>>(assInfoRaw);
		AssignableItemData data{toEnumVec(enumVec), unit};
		return createAssignable(node, conn, data);
	}
	return std::nullopt;
}

template <typename T>
void updateReadItemText(QStandardItem *item, T value, std::optional<QString> unit) {
	// TODO: this can be made a lot (around 3x) faster by using direct copying
	// Form a string of the form "1000 MHz" if has unit
	auto text = (unit.has_value()) ? QString("%1 %2").arg(value).arg(unit.value())
				       : QString("%1").arg(value);
	item->setText(text);
	// qDebug() << item->data(DeviceModel::DynamicReadableProxyRole);
}

std::optional<QStandardItem *> DeviceModel::setupDynReadable(
    TC::TreeNode<TCDBus::DeviceNode> node, QDBusConnection conn) {
	auto item = new QStandardItem;
	auto proxy = new DynamicReadableProxy(node.value().path, conn, this);
	QVariant v;
	v.setValue(proxy);
	item->setData(v, DynamicReadableProxyRole);
	auto unit = proxy->unit();

	connect(proxy, &DynamicReadableProxy::valueChanged, [=](ReadResult res) {
		p::match(res)(
		    pattern(as<ReadableValue>(arg)) =
			[=](auto rv) {
				p::match(rv)(
				    pattern(as<double>(arg)) =
					[=](auto d) { updateReadItemText(item, d, unit); },
				    pattern(as<int>(arg)) =
					[=](auto i) { updateReadItemText(item, i, unit); },
				    pattern(as<uint>(arg)) =
					[=](auto u) { updateReadItemText(item, u, unit); },
				    pattern(_) = [] {});
			},
		    pattern(_) = [] {});
	});

	return item;
}

std::optional<QStandardItem *> DeviceModel::setupStaticReadable(
    TC::TreeNode<TCDBus::DeviceNode> node, QDBusConnection conn) {
	QDBusInterface staticIface(
	    "org.tuxclocker", node.value().path, "org.tuxclocker.StaticReadable", conn);
	auto value = staticIface.property("value").value<QDBusVariant>().variant().toString();
	// Workaround from DynamicReadableProxy for getting property with custom type
	QDBusInterface propIface(
	    "org.tuxclocker", node.value().path, "org.freedesktop.DBus.Properties", conn);
	QDBusReply<QDBusVariant> reply =
	    propIface.call("Get", "org.tuxclocker.StaticReadable", "unit");
	if (!reply.isValid())
		return std::nullopt;
	auto arg = reply.value().variant().value<QDBusArgument>();
	TCDBus::Result<QString> unit;
	arg >> unit;

	if (!unit.error)
		value += " " + unit.value;

	auto item = new QStandardItem;
	item->setData(value, Qt::DisplayRole);
	return item;
}
