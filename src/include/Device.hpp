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
using ReadableValue = std::variant<int, uint, double, std::string>;
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
	DynamicReadable(const std::function<std::variant<ReadError,
			ReadableValue>()> readFunc,
			std::optional<std::string> unit = std::nullopt) {
		m_readFunc = readFunc;
		m_unit = unit;
	}
	/*std::variant<ReadError, ReadableValue>*/ ReadResult read() {
		return m_readFunc();
	}
	auto unit() {return m_unit;}
private:
	std::function<std::variant<ReadError, ReadableValue>()> m_readFunc;
	std::optional<std::string> m_unit;
};

class StaticReadable {
public:
	StaticReadable(ReadableValue value, std::optional<std::string> unit) {
		m_value = value;
		m_unit = unit;
	}
	ReadableValue value() {return m_value;}
	std::optional<std::string> unit() {return m_unit;}
private:
	ReadableValue m_value;
	std::optional<std::string> m_unit;
};

/* DeviceNode has a name, and optionally implements one of
   [Assignable, DynamicReadable, StaticReadable] */
using DeviceInterface = std::variant<Assignable, DynamicReadable, StaticReadable>;

struct DeviceNode {
	std::string name;
	std::optional<DeviceInterface> interface;
	std::string hash;
};

};
};
