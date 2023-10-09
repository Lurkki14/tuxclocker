#include <Utils.hpp>

bool hasReadableValue(TuxClocker::Device::ReadResult res) {
	return std::holds_alternative<TuxClocker::Device::ReadableValue>(res);
}
