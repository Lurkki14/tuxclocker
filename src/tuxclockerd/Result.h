#pragma once

#include <QObject>
#include <QDBusVariant>
#include <QDBusArgument>
#include <QDBusMetaType>

template <typename T>
struct Result {
    bool valid;
    T value;
    friend QDBusArgument &operator<<(QDBusArgument &arg, const Result &res) {
        arg.beginStructure();
        arg << res.valid << res.value;
        arg.endStructure();
        return arg;
    }

    friend const QDBusArgument &operator>>(const QDBusArgument &arg, Result &varRes) {
        arg.beginStructure();
        arg >> varRes.valid >> varRes.value;
        arg.endStructure();
        return arg;
    }
};

Q_DECLARE_METATYPE(Result<QDBusVariant>)
