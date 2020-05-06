#pragma once

#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace TuxClocker {
namespace Device {
	
enum class AssignmentError {
	InvalidArgument,
	InvalidType,
	NoPermission,
	OutOfRange,
	UnknownError
};

enum class ReadError {
	UnknownError
};

template <typename T>
struct Range {
	Range() {};
	Range(const T &min_, const T &max_) {min = min_, max = max_;}
	T min, max;
};
	
struct Enumeration {
	Enumeration(const std::string &name_, const uint &key_) {name = name_; key = key_;}
	std::string name;
	uint key;
};

using AssignmentArgument = std::variant<int, double, uint>;
using ReadableValue = std::variant<int, uint, double>;
using ReadResult = std::variant<ReadError, ReadableValue>;
using RangeInfo = std::variant<Range<int>, Range<double>>;
using EnumerationVec = std::vector<Enumeration>;
using AssignableInfo = std::variant<RangeInfo, std::vector<Enumeration>>;

class Assignable {
public:
	Assignable(const std::function<std::optional<AssignmentError>(AssignmentArgument)> assignmentFunc,
			AssignableInfo info) {
		m_assignmentFunc = assignmentFunc;
		m_assignableInfo = info;
	}
	std::optional<AssignmentError> assign(AssignmentArgument arg) {
		return m_assignmentFunc(arg);
	}
	AssignableInfo assignableInfo() {return m_assignableInfo;}
private:
	AssignableInfo m_assignableInfo;
	std::function<std::optional<AssignmentError>(AssignmentArgument)> m_assignmentFunc;
};

class DynamicReadable {
public:
	DynamicReadable() {}
	DynamicReadable(const std::function<std::variant<ReadError, ReadableValue>()> readFunc) {
		m_readFunc = readFunc;
	}
	/*std::variant<ReadError, ReadableValue>*/ ReadResult read() {
		return m_readFunc();
	}
private:
	std::function<std::variant<ReadError, ReadableValue>()> m_readFunc;
	std::optional<std::string> m_unit;
};

// DeviceNode has a name, and optionally implements one of [Assignable, DynamicReadable]
using DeviceInterface = std::variant<Assignable, DynamicReadable>;

struct DeviceNode {
	std::string name;
	std::optional<DeviceInterface> interface;
	std::string hash;
};

};
};
