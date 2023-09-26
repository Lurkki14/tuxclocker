#include <Utils.hpp>

#include <AssignableProxy.hpp>
#include <DeviceModel.hpp>
#include <DeviceModelDelegate.hpp>
#include <DynamicReadableConnectionData.hpp>
#include <functional>
#include <QDebug>
#include <QSettings>
#include <Settings.hpp>

namespace Utils {

QVariant fromAssignmentArgument(TuxClocker::Device::AssignmentArgument arg) {
	if (std::holds_alternative<int>(arg))
		return std::get<int>(arg);

	if (std::holds_alternative<uint>(arg))
		return std::get<uint>(arg);

	if (std::holds_alternative<double>(arg))
		return std::get<double>(arg);
	return QVariant{};
}

NodePath fromSettingsPath(QString path) { return path.replace('-', '/'); }

QString toSettingsPath(NodePath path) { return path.replace('/', '-'); }

void traverseModel(
    const ModelTraverseCallback &cb, QAbstractItemModel *model, const QModelIndex &parent) {
	// Run callback on the index itself
	(void) cb(model, parent.parent(), parent.row());

	for (int i = 0; i < model->rowCount(parent); i++) {
		auto nextOpt = cb(model, parent, i);
		if (!nextOpt.has_value())
			// Returning nothing is used as indication we should stop traversing
			return;

		// We get the next index we should traverse, and the funtion does
		// its thing with the model and index
		auto nextIndex = nextOpt.value();
		if (model->hasChildren(nextIndex)) {
			traverseModel(cb, model, nextIndex);
		}
	}
}

std::optional<QModelIndex> fromAssignablePath(DeviceModel &model, NodePath path) {
	QModelIndex assIndex{};
	// Find index from assignable path
	auto cb = [&](auto model, auto index, int row) -> std::optional<QModelIndex> {
		auto ifaceIndex = model->index(row, DeviceModel::InterfaceColumn, index);
		auto assProxyV = ifaceIndex.data(DeviceModel::AssignableProxyRole);

		if (assProxyV.isValid()) {
			auto assProxy = qvariant_cast<AssignableProxy *>(assProxyV);
			if (assProxy->dbusPath() == path) {
				assIndex = ifaceIndex;
				return std::nullopt;
			}
		}
		return model->index(row, DeviceModel::NameColumn, index);
	};
	traverseModel(cb, &model);

	if (!assIndex.isValid()) {
		qWarning("Couldn't find assignable with path %s from model!", qPrintable(path));
		return std::nullopt;
	}

	return assIndex;
}

// Write profile's assignable settings to model
void setModelAssignableSettings(DeviceModel &model, QVector<AssignableSetting> settings) {
	QVector<AssignableDefaultData> assSettings;

	for (auto &setting : settings) {
		auto index = fromAssignablePath(model, setting.assignablePath);
		if (index.has_value())
			assSettings.append(AssignableDefaultData{
			    .index = index.value(),
			    .defaultValue = setting.value,
			});
	}
	// TODO: misleading name when used here!
	DeviceModelDelegate::setAssignableDefaults(&model, assSettings);
}

void writeAssignableDefaults(DeviceModel &model) {
	ModelTraverseCallback cb = [](QAbstractItemModel *model, QModelIndex index, int row) {
		auto ifaceIndex = model->index(row, DeviceModel::InterfaceColumn, index);

		auto assProxyV = ifaceIndex.data(DeviceModel::AssignableProxyRole);

		if (assProxyV.isValid()) {
			auto assProxy = qvariant_cast<AssignableProxy *>(assProxyV);
			auto currentValue = assProxy->currentValue();

			if (currentValue.has_value()) {
				auto currentValueV = fromAssignmentArgument(currentValue.value());

				QSettings settings{"tuxclocker"};
				settings.beginGroup("assignableDefaults");

				// QSettings doesn't want us to use slashes for keys
				auto settingsPath = toSettingsPath(assProxy->dbusPath());
				// Don't set again, so the program can be closed
				// with assignables changed, and defaults still be set
				if (!settings.contains(settingsPath))
					settings.setValue(settingsPath, currentValueV);

				settings.endGroup();
			}
		}
		auto nextIndex = model->index(row, DeviceModel::NameColumn, index);

		return nextIndex;
	};
	traverseModel(cb, &model);
}

void writeAssignableSetting(SettingsData data, AssignableSetting setting) {
	qRegisterMetaType<DynamicReadableConnectionData>();

	qRegisterMetaTypeStreamOperators<QVector<QPointF>>("QVector<QPointF>");
	qRegisterMetaTypeStreamOperators<DynamicReadableConnectionData>(
	    "DynamicReadableConnectionData");

	if (!data.currentProfile.has_value())
		return;

	QSettings settings{"tuxclocker"};
	auto profile = data.currentProfile.value();
	settings.beginGroup("profiles");
	settings.beginGroup(profile);
	settings.setValue(toSettingsPath(setting.assignablePath), setting.value);
}

} // namespace Utils
