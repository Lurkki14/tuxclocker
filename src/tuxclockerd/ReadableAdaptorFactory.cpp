#include "ReadableAdaptorFactory.h"

QDBusAbstractAdaptor *ReadableAdaptorFactory::readableAdaptor(QObject *parent, const tc_readable_node_t *node) {
	if (!node->value_callback) {
		return nullptr;
	}
	return new ReadableDynamicVariantAdaptor(parent, node);
}
