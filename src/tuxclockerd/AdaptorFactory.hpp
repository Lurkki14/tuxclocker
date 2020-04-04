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
	static std::optional<QDBusAbstractAdaptor*> adaptor(QObject *obj,
			DeviceInterface iface) {
		std::optional<QDBusAbstractAdaptor*> retval = std::nullopt;
		match(iface)
			(pattern(as<DynamicReadable>(arg)) = [&](auto dr) {
				qDebug() << "making rd adaptor";
				retval = new DynamicReadableAdaptor(obj, dr);
			},
			(pattern(_)) = [] {});
		return retval;
	}
};
