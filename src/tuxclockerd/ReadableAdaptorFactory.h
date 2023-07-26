#pragma once

#include <QObject>
#include <QDBusAbstractAdaptor>
#include <tc_readable.h>

#include "ReadableDynamicVariantAdaptor.h"

class ReadableAdaptorFactory {
public:
	static QDBusAbstractAdaptor *readableAdaptor(
	    QObject *parent, const tc_readable_node_t *node);
};
