// SPDX-FileCopyrightText: 2016 Sandro Knauß <knauss@kolabsys.com>
// SPDX-FileCopyCopyright: 2017 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <QAbstractTableModel>
#include <QModelIndex>

#include <memory>

namespace MimeTreeParser
{
class ObjectTreeParser;
}
class AttachmentModelPrivate;

class AttachmentModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    AttachmentModel(std::shared_ptr<MimeTreeParser::ObjectTreeParser> parser);
    ~AttachmentModel();

public:
    enum Roles {
        TypeRole = Qt::UserRole + 1,
        IconRole,
        NameRole,
        SizeRole,
        IsEncryptedRole,
        IsSignedRole,
    };

    enum Columns { NameColumn = 0, SizeColumn, IsEncryptedColumn, IsSignedColumn, ColumnCount };

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE bool saveAttachmentToDisk(const QModelIndex &parent);
    Q_INVOKABLE bool openAttachment(const QModelIndex &index);

    Q_INVOKABLE bool importPublicKey(const QModelIndex &index);

private:
    std::unique_ptr<AttachmentModelPrivate> d;
};
