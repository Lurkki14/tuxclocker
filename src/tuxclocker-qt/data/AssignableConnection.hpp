#pragma once

#include <boost/signals2.hpp>
#include <functional>
#include <QObject>
#include <QVariant>

// Interface for the function for an assignable
class AssignableConnection : public QObject {
public:
	AssignableConnection(QObject *parent = nullptr) : QObject(parent) {}
	/*AssignableConnection(const AssignableConnection &other) {
		targetValueChanged.connect(other.targetValueChanged);
	}*/
	virtual ~AssignableConnection() {}
	// Returns the data as QVariant for saving to settings
	virtual QVariant connectionData() = 0;
	virtual void start() = 0;
	virtual void stop() = 0;
	/* Value and text representation of desired value,
	   since converting QDBusVariant to QString isn't trivial
	   for complex types. */
	/*boost::signals2::signal<void(QVariant, QString)> targetValueChanged;
	boost::signals2::signal<void()> started;
	boost::signals2::signal<void()> stopped;*/
signals:
	void targetValueChanged(QVariant, QString);
	void started();
	void stopped();
private:
	Q_OBJECT
};
