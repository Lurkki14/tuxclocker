#pragma once

#include <DeviceModel.hpp>

namespace Utils {

using ModelTraverseCallback =
    std::function<std::optional<QModelIndex>(QAbstractItemModel *, const QModelIndex &, int)>;
using NodePath = QString;

// Conversion for saving in settings
NodePath fromSettingsPath(QString);
QString toSettingsPath(NodePath);

void traverseModel(
    const ModelTraverseCallback &, QAbstractItemModel *, const QModelIndex &parent = QModelIndex());
void writeAssignableDefaults(DeviceModel &model);

} // namespace Utils
