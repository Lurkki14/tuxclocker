#pragma once

#include <QStandardItemModel>
#include <QMimeData>
#include <QDebug>

#include <ReadableData.h>

class ReadableItemModel : public QStandardItemModel {
public:
    ReadableItemModel() : QStandardItemModel() {}
protected:
    QStringList mimeTypes() const override {return QStringList(ReadableData::mimeType());}
    QMimeData *mimeData(const QModelIndexList &indices) const override {
        // FIXME: Pretty messy to have this here
        qRegisterMetaTypeStreamOperators<ReadableData>("ReadableData");
        
        if (indices.size() < 1) {
            return nullptr;
        }
        QByteArray b_data;
        QDataStream stream(&b_data, QIODevice::WriteOnly);
        
        stream << itemFromIndex(indices[0])->data(Qt::UserRole);
        
        QMimeData *data = new QMimeData;
        data->setData(ReadableData::mimeType(), b_data);
        
        return data;
    }
};
