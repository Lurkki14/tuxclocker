#pragma once

#include <DeviceModel.hpp>

namespace Utils {

using ModelTraverseCallback =
    std::function<QModelIndex(QAbstractItemModel *, const QModelIndex &, int)>;

void traverseModel(
    const ModelTraverseCallback &, QAbstractItemModel *, const QModelIndex &parent = QModelIndex());
void writeAssignableDefaults(DeviceModel &model);

} // namespace Utils
