#pragma once

#include <DeviceModel.hpp>

namespace Utils {

using ModelTraverseCallback = std::function<QModelIndex(QAbstractItemModel *, QModelIndex &, int)>;

void traverseModel(
    const ModelTraverseCallback &, QAbstractItemModel *, QModelIndex parent = QModelIndex());
void writeAssignableDefaults(DeviceModel &model);

} // namespace Utils
