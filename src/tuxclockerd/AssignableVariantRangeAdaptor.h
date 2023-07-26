#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusVariant>
#include <tc_assignable.h>

class AssignableVariantRangeAdaptor : public QDBusAbstractAdaptor {
public:
	explicit AssignableVariantRangeAdaptor(QObject *obj, const tc_assignable_node_t *node);
	QString name_() { return QString(m_node->name); }
	QString unit_() { return QString(m_node->unit); }
	QDBusVariant min_();
	QDBusVariant max_();
public Q_SLOTS:
	short assign(QDBusVariant value);
private:
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.tuxclocker.Assignable.Range.Variant")
	Q_PROPERTY(QString name READ name_)
	Q_PROPERTY(QString unit READ unit_)
	Q_PROPERTY(QDBusVariant min READ min_)
	Q_PROPERTY(QDBusVariant max READ max_)

	const tc_assignable_node_t *m_node;
};
