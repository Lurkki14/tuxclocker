#include <AMDUtils.hpp>
#include <fplus/fplus.hpp>
#include <iostream>
#include <Utils.hpp>

using namespace TuxClocker::Device;

std::optional<Range<int>> fromFanCurveContents(const std::string &contentsRaw) {
	auto contents =
	    fplus::replace_tokens(std::string{"fan speed"}, std::string{"fan_speed"}, contentsRaw);
	return parsePstateRangeLine("FAN_CURVE(fan_speed)", contents);
}

int fanCurveParse() {
	auto contents = fileContents(PROJECT_ROOT "/doc/amd-pptables/rx7000-fancurve");
	if (!contents.has_value()) {
		std::cerr << "Couldn't read sample file\n";
		return 1;
	}

	auto range = fromFanCurveContents(*contents);
	if (!range.has_value()) {
		return 1;
	}
	return !(range->min == 15 && range->max == 100);
}

int main() { return fanCurveParse(); }
