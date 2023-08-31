#include <Utils.hpp>

#include <AssignableProxy.hpp>
#include <DeviceModel.hpp>
#include <functional>
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
				auto dbusPath = assProxy->dbusPath().replace('/', '-');
				// Don't set again, so the program can be closed
				// with assignables changed, and defaults still be set
				if (!settings.contains(dbusPath))
					settings.setValue(dbusPath, currentValueV);

				settings.endGroup();
			}
		}
		auto nextIndex = model->index(row, DeviceModel::NameColumn, index);

		return nextIndex;
	};
	traverseModel(cb, &model);
}

} // namespace Utils
