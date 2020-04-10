#pragma once

#include "Adaptors.hpp"
#include <DBusTypes.hpp>
#include <Device.hpp>

#include <patterns.hpp>
#include <QDBusAbstractAdaptor>
#include <QDBusArgument>
#include <QDBusVariant>
#include <QDBusMetaType>

using namespace TuxClocker::Device;
using namespace mpark::patterns;

namespace TCDBus = TuxClocker::DBus;

Q_DECLARE_METATYPE(TCDBus::ReadResult)

class DynamicReadableAdaptor : public QDBusAbstractAdaptor {
public:
	explicit DynamicReadableAdaptor(QObject *obj, DynamicReadable dr) : QDBusAbstractAdaptor(obj) {
		// Ideally this should be moved somewhere else but QMetaType does not handle namespaces well
		qDBusRegisterMetaType<TCDBus::ReadResult>();
		m_dynamicReadable = dr;
	}
public Q_SLOTS:
	TCDBus::ReadResult value() {
		QVariant v;
		TCDBus::ReadResult res{.error = false};
		/* We have to unwrap the value here, another option would be to convert the std::variant
		 * to char*, but that comes with its own issues */
		match(m_dynamicReadable.read())
			(pattern(as<ReadableValue>(arg)) = [&](auto val) {
				match(val)
					(pattern(as<uint>(arg)) = [&](auto u) {
						v.setValue(u);
					},
					pattern(as<double>(arg)) = [&](auto d) {
						v.setValue(d);
					});
			},
			pattern(as<ReadError>(arg)) = [&](auto err) {
				v.setValue(static_cast<int>(err));
				res.error = true;
			});
		res.value = QDBusVariant(v);
		return res;
	}
private:
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.tuxclocker.DynamicReadable")

	DynamicReadable m_dynamicReadable;
};
