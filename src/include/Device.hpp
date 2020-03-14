#pragma once

#include <string>
#include <variant>
#include <vector>

namespace TuxClocker {
namespace Device {
	
template <typename T>
struct Range {
	Range();
	Range(const T &min_, const T &max_) {min = min_, max = max_;}
	T min, max;
};
	
struct Enumeration {
	Enumeration(const std::string &name_, const uint &key_) {name = name_; key = key_;}
	std::string name;
	uint key;
};

using RangeInfo = std::variant<Range<int>, Range<double>>;
using AssignableInfo = std::variant<RangeInfo, std::vector<Enumeration>>;

class Assignable {
public:
	Assignable();
private:
	
};
	
};
};
