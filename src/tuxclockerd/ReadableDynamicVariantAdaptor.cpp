#include "ReadableDynamicVariantAdaptor.h"

ReadableDynamicVariantAdaptor::ReadableDynamicVariantAdaptor(
    QObject *obj, const tc_readable_node_t *node)
    : QDBusAbstractAdaptor(obj) {
	m_node = node;
}

Result<QDBusVariant> ReadableDynamicVariantAdaptor::value() {
	QDBusVariant dv;
	QVariant v;

	auto result = m_node->value_callback(m_node);
	switch (result.data.data_type) {
	case TC_TYPE_DOUBLE:
		v.setValue(result.data.double_value);
		break;
	case TC_TYPE_INT:
		v.setValue(result.data.int_value);
		break;
	case TC_TYPE_UINT: {
		qulonglong value = result.data.uint_value;
		v.setValue(value);
		break;
	}
	default:
		break;
	}

	dv.setVariant(v);

	Result<QDBusVariant> res = {.valid = result.valid, .value = dv};

	return res;
}
