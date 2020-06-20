#include "DeviceProxyModel.hpp"

#include <QDebug>

Q_DECLARE_METATYPE(DeviceModel::InterfaceFlag)

DeviceProxyModel::DeviceProxyModel(DeviceModel &model, QObject *parent) :
		QSortFilterProxyModel(parent), m_showIcons(true), m_showValueColumn(true) {
	setSourceModel(&model);
	m_flags = DeviceModel::AllInterfaces;
}

bool DeviceProxyModel::filterAcceptsRow(int sourceRow,
		const QModelIndex &sourceParent) const {
	auto model = sourceModel();
	// Interface type is stored in the item with the name
	auto thisItem = model->index(sourceRow, DeviceModel::NameColumn,
		sourceParent);
	
	// Check recursively if a child item has the flag set that we want to show
	bool shouldHide = true;
	std::function<void(QModelIndex)> traverse;
	traverse = [&](QModelIndex index) {
		auto data = index.data(DeviceModel::InterfaceTypeRole);
		auto ifaceType = data.value<DeviceModel::InterfaceFlag>();
		if (data.isValid() && (m_flags & ifaceType)) {
			// Item has the flag we want to show
			shouldHide = false;
			return;
		}
		auto rowCount = model->rowCount(index);
		for (int i = 0; i < rowCount; i++)
			traverse(model->index(i, DeviceModel::NameColumn, index));
	};
	traverse(thisItem);

	return !shouldHide;
}

bool DeviceProxyModel::lessThan(const QModelIndex &left,
		const QModelIndex &right) const {
	// TODO: doesn't work as expected if sorted from interface column
	auto leftChildren = sourceModel()->rowCount(left);
	auto rightChildren = sourceModel()->rowCount(right);

	if (leftChildren == 0 && rightChildren != 0)
		// Left hand side is 'smaller' if right has children
		return true;
	return QSortFilterProxyModel::lessThan(left, right);
}
