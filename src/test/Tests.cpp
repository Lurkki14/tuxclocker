#include <AMDUtils.hpp>
#include <fplus/fplus.hpp>
#include <iostream>
#include <Utils.hpp>

using namespace TuxClocker::Device;

const char *fanCurvePath = PROJECT_ROOT "/doc/amd-pptables/rx7000-fancurve";
const char *fileErrorMessage = "Couldn't read sample file";

int test(std::vector<std::function<int()>> funcs) {
	for (int i = 0; i < funcs.size() - 1; i++) {
		auto ret = funcs[i]();
		if (ret != 0)
			return ret;
	}
	return funcs.back()();
}

int failWith(const char *message) {
	std::cerr << message << "\n";
	return 1;
}

int fanCurveSpeedRangeParse() {
	auto contents = fileContents(fanCurvePath);
	if (!contents.has_value())
		return failWith(fileErrorMessage);

	auto range = speedRangeFromContents(*contents);
	if (!range.has_value())
		return failWith("Couldn't parse fan speed range");

	return !(range->min == 15 && range->max == 100);
}

int fanCurveTempRangeParse() {
	auto contents = fileContents(fanCurvePath);
	if (!contents.has_value())
		return failWith(fileErrorMessage);

	auto range = tempRangeFromContents(*contents);
	if (!range.has_value())
		return failWith("Couldn't parse temperature range");

	return !(range->min == 25 && range->max == 100);
}

int fanCurvePointParse() {
	auto contents = fileContents(fanCurvePath);
	if (!contents.has_value())
		return failWith(fileErrorMessage);

	std::vector<int> expectedTemps = {0, 45, 50, 55, 65};
	auto temps = fanCurveTempsFromContents(*contents);
	return !(expectedTemps == temps);
}

int main() {
	return test({fanCurveSpeedRangeParse, fanCurveTempRangeParse, fanCurvePointParse});
}
