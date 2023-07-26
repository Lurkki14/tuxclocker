#include "AssignableEnumAdaptor.h"

AssignableEnumAdaptor::AssignableEnumAdaptor(QObject *obj, const tc_assignable_node_t *node)
    : QDBusAbstractAdaptor(obj) {
	m_node = node;
}

QList<EnumData> AssignableEnumAdaptor::values_() {
	QList<EnumData> retval;

	for (uint16_t i = 0; i < m_node->enum_info.property_count; i++) {
		EnumData ed = {.name = QString(m_node->enum_info.properties[i]), .value = i};
		retval.append(ed);
	}
	return retval;
}

short AssignableEnumAdaptor::assign(ushort value) {
	tc_variant_t tc_v;

	tc_v.uint_value = value;
	tc_v.data_type = TC_TYPE_UINT;

	assert(m_node->assign_callback);

	return m_node->assign_callback(tc_v, m_node);
}
