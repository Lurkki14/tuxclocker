#pragma once

#include <tc_assignable.h>
#include <QDBusAbstractAdaptor>

class AssignableAdaptorFactory {
public:
	static QDBusAbstractAdaptor *assignableAdaptor(QObject *parent, const tc_assignable_node_t *node);
};
