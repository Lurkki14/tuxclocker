#pragma once

#include <QDBusAbstractAdaptor>
#include <tc_assignable.h>

#include "EnumData.h"

class AssignableEnumAdaptor : public QDBusAbstractAdaptor {
public:
	explicit AssignableEnumAdaptor(QObject *obj, const tc_assignable_node_t* node);
	QList <EnumData> values_();
public Q_SLOTS:
	short assign(ushort value);
private:
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.tuxclocker.Assignable.Enum")
	Q_PROPERTY(QList <EnumData> values READ values_)
	
	const tc_assignable_node_t *m_node;
};
