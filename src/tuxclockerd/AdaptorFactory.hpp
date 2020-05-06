#pragma once

#include "Adaptors.hpp"
#include <Device.hpp>

#include <memory>
#include <patterns.hpp>
#include <QDebug>
#include <QDBusAbstractAdaptor>

using namespace TuxClocker::Device;
using namespace mpark::patterns;

class AdaptorFactory {
public:
	/* Returns a raw pointer since smart pointers caused some scope issues.
	 * TODO: try to use smart pointers instead? */
	static std::optional<QDBusAbstractAdaptor*> adaptor(QObject *obj,
			DeviceInterface iface, DeviceNode devNode) {
		std::optional<QDBusAbstractAdaptor*> retval = std::nullopt;
		match(iface)
			(pattern(as<DynamicReadable>(arg)) = [&](auto dr) {
				retval = new DynamicReadableAdaptor(obj, dr);
			},
			pattern(as<Assignable>(arg)) = [&](auto a) {
				retval = new AssignableAdaptor(obj, a, devNode);
			},
			pattern(_) = []{});
		return retval;
	}
};
