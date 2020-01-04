#include "AssignableAdaptorFactory.h"

#include "AssignableEnumAdaptor.h"
#include "AssignableVariantRangeAdaptor.h"

#include <QDebug>

QDBusAbstractAdaptor *AssignableAdaptorFactory::assignableAdaptor(QObject *parent, const tc_assignable_node_t *node) {
	switch (node->value_category) {
		case TC_ASSIGNABLE_RANGE:
			return new AssignableVariantRangeAdaptor(parent, node);
		case TC_ASSIGNABLE_ENUM:
			qDebug() << "using enum adaptor";
			return new AssignableEnumAdaptor(parent, node);
		default:
			return nullptr;
	}
	return nullptr;
}
