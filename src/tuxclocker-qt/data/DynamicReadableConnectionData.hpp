#pragma once

#include <Device.hpp>
#include <QDataStream>
#include <QMetaType>
#include <QPointF>
#include <QString>
#include <QVector>

struct DynamicReadableConnectionData {
	// y = f(x)
	QVector<QPointF> points;
	// DBus path
	QString dynamicReadablePath;
	TuxClocker::Device::RangeInfo rangeInfo;

	friend bool operator==(
	    const DynamicReadableConnectionData &, const DynamicReadableConnectionData &);
	friend QDataStream &operator<<(QDataStream &, const DynamicReadableConnectionData &);
	friend QDataStream &operator>>(QDataStream &, DynamicReadableConnectionData &);
};
Q_DECLARE_METATYPE(DynamicReadableConnectionData);
