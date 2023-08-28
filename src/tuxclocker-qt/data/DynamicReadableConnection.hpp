#pragma once

#include "AssignableConnection.hpp"
#include <DynamicReadableProxy.hpp>
#include <fplus/fplus.hpp>
#include <patterns.hpp>
#include <QDBusVariant>
#include <QDebug>
#include <QPointF>
#include <QRandomGenerator>
#include <QTimer>
#include <QVector>
#include <memory>

// delete these
using namespace fplus;
using namespace mpark::patterns;
using namespace TuxClocker::Device;

class DynamicReadableConnectionData {
public:
	// y = f(x)
	QVector<QPointF> points;
	// DBus path
	QString dynamicReadablePath;
};

Q_DECLARE_METATYPE(DynamicReadableConnectionData)

// TODO: make DynamicReadableProxy type parametrized so we can be sure to only get numeric values
// from it Connection of an Assignable with a DynamicReadable
template <typename OutType> // Result of linear interpolation
class DynamicReadableConnection : public AssignableConnection {
public:
	DynamicReadableConnection(
	    DynamicReadableProxy &proxy, QVector<QPointF> points, QObject *parent = nullptr)
	    : AssignableConnection(parent), m_proxy(proxy), m_points(points) {
		auto sorted = sort_by(
		    [](auto point_l, auto point_r) { return point_l.x() < point_r.x(); }, m_points);
		m_points = sorted;
	}
	virtual QVariant connectionData() override { return QVariant(); }
	virtual void start() override {
		// TODO: Something goes wrong here (not always!!)
		connect(&m_proxy, &DynamicReadableProxy::valueChanged, [this](auto val) {
			match(val)(pattern(as<ReadableValue>(arg)) = [this](auto rv) {
				match(rv)(pattern(as<uint>(arg)) = [this](auto u) {
					emitTargetValue(u);
				});
			});
		});

		/*QDBusVariant arg{QVariant{1}};
		QVariant v;
		v.setValue(arg);
		emit targetValueChanged(v, "1");*/
	}
	virtual void stop() override { disconnect(&m_proxy, nullptr, nullptr, nullptr); }
private:
	DynamicReadableProxy &m_proxy;
	QTimer m_timer;
	QVector<QPointF> m_points;

	template <typename U> void emitTargetValue(U reading) {
		// Find two points from the vector so that:
		//   p[i].x < val < p[i + 1].x
		std::optional<int> leftIndex = std::nullopt;
		for (int i = 0; i < m_points.length() - 1; i++) {
			if (m_points[i].x() < reading && reading < m_points[i + 1].x()) {
				leftIndex = i;
				break;
			}
		}
		if (leftIndex == std::nullopt)
			return;
		int li = leftIndex.value();
		// What percentage the value is from dx of left and right interp points
		double dx = m_points[li + 1].x() - m_points[li].x();
		double dvx = reading - m_points[li].x();
		double p = dvx / dx;
		OutType interp_y = lerp(m_points[li].y(), m_points[li + 1].y(), p);
		QDBusVariant arg{QVariant{interp_y}};
		QVariant v;
		v.setValue(arg);
		emit targetValueChanged(v, QString::number(interp_y));
	}

	template <typename T> T lerp(T a, T b, double t) { return a + (t * (b - a)); }
};
