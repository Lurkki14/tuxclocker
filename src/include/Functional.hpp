#pragma once

#include <functional>
#include <tuple>

namespace TuxClocker {

// Creates a function with no arguments from a function that takes some from inputs to the function and the function
template<typename T, typename... Args>
auto specializeFunction(Args... args, std::function<T(Args...)> func) {
        return [args = std::make_tuple(std::move(args)...), &func]() {
                return std::apply([&func](auto ...args) {
                        func(args...);
                }, std::move(args));
        };
}

template<template<typename> typename List, typename In, typename Out>
List<Out> map(List<In> list, std::function<Out(In)> func) {
	List<Out> retval;
	std::transform(list.begin(), list.end(), std::back_inserter(retval), func);
	return retval;
}

template<typename List, typename T>
List filter(List list, std::function<bool(T)> func) {
	List retval;
	std::copy_if(list.begin(), list.end(), std::back_inserter(retval), func);
	return retval;
}

};
