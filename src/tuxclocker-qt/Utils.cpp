#include <Utils.hpp>

#include <AssignableProxy.hpp>
#include <DeviceModel.hpp>
#include <QDebug>
#include <QSettings>

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

void traverseModel(const QAbstractItemModel *model, QModelIndex parent = QModelIndex()) {
	for (int i = 0; i < model->rowCount(parent); i++) {
		auto index = model->index(i, DeviceModel::InterfaceColumn, parent);
		// Only this index will have children
		auto nameIndex = model->index(i, DeviceModel::NameColumn, parent);

		auto assProxyV = index.data(DeviceModel::AssignableProxyRole);

		if (assProxyV.isValid()) {
			auto assProxy = qvariant_cast<AssignableProxy *>(assProxyV);
			auto currentValue = assProxy->currentValue();

			if (currentValue.has_value()) {
				auto currentValueV = fromAssignmentArgument(currentValue.value());

				QSettings settings{"tuxclocker"};
				settings.beginGroup("assignableDefaults");

				// QSettings doesn't want us to use slashes for keys
				auto dbusPath = assProxy->dbusPath().replace('/', '-');
				// Don't set again, so the program can be closed
				// with assignables changed, and defaults still be set
				if (!settings.contains(dbusPath))
					settings.setValue(dbusPath, currentValueV);

				settings.endGroup();
			}
		}

		if (model->hasChildren(nameIndex))
			traverseModel(model, nameIndex);
	}
}

void writeAssignableDefaults(const DeviceModel &model) { traverseModel(&model); }

} // namespace Utils
