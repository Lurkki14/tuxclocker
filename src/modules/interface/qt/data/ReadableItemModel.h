#pragma once

#include <QStandardItemModel>
#include <QMimeData>

#include <ReadableData.h>

class ReadableItemModel : public QStandardItemModel {
public:
    ReadableItemModel() : QStandardItemModel() {}
protected:
    QStringList mimeTypes() const override {return QStringList(ReadableData::mimeType());}
    QMimeData *mimeData(const QModelIndexList &indices) const override {
        if (indices.size() < 1) {
            return nullptr;
        }
        
        QVariantList itemDataList;
        
        for (QModelIndex modelIndex : indices) {
            itemDataList.append(itemFromIndex(modelIndex)->data(Qt::UserRole));
        }
        
        QByteArray b_data;
        QDataStream stream(&b_data, QIODevice::WriteOnly);
        
        stream << itemDataList;
        
        QMimeData *data = new QMimeData;
        data->setData(ReadableData::mimeType(), b_data);
        
        return data;
    }
};
