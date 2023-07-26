#pragma once

#include <QDBusAbstractAdaptor>
#include <tc_readable.h>

#include "Result.h"

class ReadableDynamicVariantAdaptor : public QDBusAbstractAdaptor {
public:
	explicit ReadableDynamicVariantAdaptor(QObject *obj, const tc_readable_node_t *node);
	QString name_() { return QString(m_node->name); }
public Q_SLOTS:
	Result<QDBusVariant> value();
private:
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.tuxclocker.Readable.Dynamic.Variant")
	Q_PROPERTY(QString name READ name_)

	const tc_readable_node_t *m_node;
};
