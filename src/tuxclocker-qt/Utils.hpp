#pragma once

#include <DeviceModel.hpp>

class AssignableSetting;
class SettingsData;

namespace Utils {

using ModelTraverseCallback =
    std::function<std::optional<QModelIndex>(QAbstractItemModel *, const QModelIndex &, int)>;
using NodePath = QString;

// File path to save non-setting persistent data, eg. window geometry
QString cacheFilePath();

// Conversion for saving in settings
NodePath fromSettingsPath(QString);
QString toSettingsPath(NodePath);

void traverseModel(
    const ModelTraverseCallback &, QAbstractItemModel *, const QModelIndex &parent = QModelIndex());
void writeAssignableDefaults(DeviceModel &model);
void writeAssignableSetting(SettingsData, AssignableSetting);
void setModelAssignableSettings(DeviceModel &model, QVector<AssignableSetting>);

} // namespace Utils
