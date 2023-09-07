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

struct DynamicReadableConnectionData {
	// y = f(x)
	QVector<QPointF> points;
	// DBus path
	QString dynamicReadablePath;
	RangeInfo rangeInfo;
};

enum class TargetType {
	IntType,
	DoubleType
};

Q_DECLARE_METATYPE(DynamicReadableConnectionData)

// TODO: make DynamicReadableProxy type parametrized so we can be sure to only get numeric values
// from it Connection of an Assignable with a DynamicReadable
template <typename OutType> // Result of linear interpolation
class DynamicReadableConnection : public AssignableConnection {
public:
	DynamicReadableConnection(DynamicReadableProxy &proxy, DynamicReadableConnectionData data,
	    QObject *parent = nullptr)
	    : AssignableConnection(parent), m_proxy(proxy), m_points(data.points) {
		if (std::holds_alternative<Range<int>>(data.rangeInfo))
			m_targetType = TargetType::IntType;
		else
			m_targetType = TargetType::DoubleType;

		auto sorted = sort_by(
		    [](auto point_l, auto point_r) { return point_l.x() < point_r.x(); }, m_points);
		m_points = sorted;
	}
	virtual QVariant connectionData() override { return QVariant(); }
	virtual void start() override {
		connect(&m_proxy, &DynamicReadableProxy::valueChanged, [this](auto val) {
			match(val)(pattern(as<ReadableValue>(arg)) = [this](auto rv) {
				match(rv)(
				    pattern(as<uint>(arg)) = [this](auto u) { emitTargetValue(u); },
				    pattern(as<int>(arg)) = [this](int i) { emitTargetValue(i); },
				    pattern(as<double>(arg)) = [this](
								   auto d) { emitTargetValue(d); });
			});
		});
	}
	virtual void stop() override { disconnect(&m_proxy, nullptr, nullptr, nullptr); }
private:
	TargetType m_targetType;
	DynamicReadableProxy &m_proxy;
	QTimer m_timer;
	QVector<QPointF> m_points;

	template <typename T> void doEmit(T targetValue, QString targetText) {
		QDBusVariant arg{QVariant{targetValue}};
		QVariant v;
		v.setValue(arg);
		emit targetValueChanged(v, targetText);
	}

	template <typename T> void emitWithType(T value) {
		switch (m_targetType) {
		case TargetType::IntType: {
			int target = static_cast<int>(value);
			doEmit(target, QString::number(target));
			return;
		}
		case TargetType::DoubleType: {
			auto target = static_cast<double>(value);
			// TODO: don't hardcode precision in different places
			doEmit(target, QString::number(target, 'f', 2));
			return;
		}
		}
	}

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

		if (!leftIndex.has_value()) {
			// Reading wasn't between points
			if (reading > m_points.last().y()) {
				// Use y of rightmost point
				emitWithType(m_points.last().y());
				return;
			} else if (reading < m_points.last().y()) {
				// Leftmost
				emitWithType(m_points.last().y());
				return;
			}
		}

		if (leftIndex == std::nullopt) {
			qWarning("Couldn't calculate target value from reading %s!",
			    qPrintable(QString::number(reading)));
			return;
		}
		int li = leftIndex.value();
		// What percentage the value is from dx of left and right interp points
		double dx = m_points[li + 1].x() - m_points[li].x();
		double dvx = reading - m_points[li].x();
		double p = dvx / dx;
		U interp_y = lerp(m_points[li].y(), m_points[li + 1].y(), p);

		emitWithType(interp_y);
	}

	template <typename T> T lerp(T a, T b, double t) { return a + (t * (b - a)); }
};
