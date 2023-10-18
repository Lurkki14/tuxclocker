#include "DynamicReadableConnectionData.hpp"

#include <QDataStream>

enum RangeType {
	IntRange,
	DoubleRange
};

QDataStream &operator<<(QDataStream &out, const TuxClocker::Device::RangeInfo &info) {
	if (std::holds_alternative<TuxClocker::Device::Range<int>>(info)) {
		auto intRange = std::get<TuxClocker::Device::Range<int>>(info);
		out << IntRange << intRange.max << intRange.min;
		return out;
	}

	if (std::holds_alternative<TuxClocker::Device::Range<double>>(info)) {
		auto doubleRange = std::get<TuxClocker::Device::Range<double>>(info);
		out << DoubleRange << doubleRange.max << doubleRange.min;
		return out;
	}
	return out;
}

QDataStream &operator>>(QDataStream &in, TuxClocker::Device::RangeInfo &info) {
	RangeType type;
	in >> type;
	if (type == IntRange) {
		TuxClocker::Device::Range<int> intRange;
		in >> intRange.max >> intRange.min;
		return in;
	}

	if (type == DoubleRange) {
		TuxClocker::Device::Range<double> doubleRange;
		in >> doubleRange.max >> doubleRange.min;
		return in;
	}
	return in;
}

QDataStream &operator<<(QDataStream &out, const DynamicReadableConnectionData &data) {
	out << data.dynamicReadablePath << data.points << data.rangeInfo;
	return out;
}
QDataStream &operator>>(QDataStream &in, DynamicReadableConnectionData &data) {
	in >> data.dynamicReadablePath >> data.points >> data.rangeInfo;
	return in;
}

// Comparing just the path should be enough for our purposes
bool operator==(
    const DynamicReadableConnectionData &lhs, const DynamicReadableConnectionData &rhs) {
	return lhs.dynamicReadablePath == rhs.dynamicReadablePath;
}
