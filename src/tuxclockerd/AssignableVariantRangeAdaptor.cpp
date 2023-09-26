#include "AssignableVariantRangeAdaptor.h"

AssignableVariantRangeAdaptor::AssignableVariantRangeAdaptor(
    QObject *obj, const tc_assignable_node_t *node)
    : QDBusAbstractAdaptor(obj) {
	m_node = node;
}

QDBusVariant AssignableVariantRangeAdaptor::min_() {
	QVariant v;
	QDBusVariant dv;

	switch (m_node->range_info.range_data_type) {
	case TC_ASSIGNABLE_RANGE_INT: {
		qlonglong val = m_node->range_info.int_range.min;
		v.setValue(val);
		break;
	}
	case TC_ASSIGNABLE_RANGE_DOUBLE:
		v.setValue(m_node->range_info.double_range.min);
		break;
	default:
		break;
	}
	dv.setVariant(v);
	return dv;
}

QDBusVariant AssignableVariantRangeAdaptor::max_() {
	QVariant v;
	QDBusVariant dv;

	switch (m_node->range_info.range_data_type) {
	case TC_ASSIGNABLE_RANGE_INT: {
		qlonglong val = m_node->range_info.int_range.max;
		v.setValue(val);
		break;
	}
	case TC_ASSIGNABLE_RANGE_DOUBLE:
		v.setValue(m_node->range_info.double_range.max);
		break;
	default:
		break;
	}
	dv.setVariant(v);
	return dv;
}

short AssignableVariantRangeAdaptor::assign(QDBusVariant value) {
	auto v = value.variant();

	// Convert to tc_variant_t
	tc_variant_t tc_v;

	if (v.canConvert<qlonglong>() &&
	    m_node->range_info.range_data_type == TC_ASSIGNABLE_RANGE_INT) {
		tc_v.int_value = qvariant_cast<qlonglong>(v);
		tc_v.data_type = TC_TYPE_INT;
		return m_node->assign_callback(tc_v, m_node);
	}

	if (v.canConvert<double>() &&
	    m_node->range_info.range_data_type == TC_ASSIGNABLE_RANGE_DOUBLE) {
		tc_v.double_value = qvariant_cast<double>(v);
		tc_v.data_type = TC_TYPE_DOUBLE;
		return m_node->assign_callback(tc_v, m_node);
	}

	return TC_EGENERIC;
}
