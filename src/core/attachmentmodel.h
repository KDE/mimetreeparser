// SPDX-FileCopyrightText: 2016 Sandro Knauß <knauss@kolabsys.com>
// SPDX-FileCopyCopyright: 2017 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "mimetreeparser_core_export.h"
#include "objecttreeparser.h"
#include <QAbstractTableModel>
#include <QModelIndex>

#include <memory>

namespace MimeTreeParser
{
class ObjectTreeParser;
}
class AttachmentModelPrivate;

class MIMETREEPARSER_CORE_EXPORT AttachmentModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    AttachmentModel(std::shared_ptr<MimeTreeParser::ObjectTreeParser> parser);
    ~AttachmentModel() override;

public:
    enum Roles {
        TypeRole = Qt::UserRole + 1,
        IconRole,
        NameRole,
        SizeRole,
        IsEncryptedRole,
        IsSignedRole,
        AttachmentPartRole,
    };

    enum Columns {
        NameColumn = 0,
        SizeColumn,
        IsEncryptedColumn,
        IsSignedColumn,
        ColumnCount,
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE bool openAttachment(const int row);
    Q_INVOKABLE bool importPublicKey(const int row);

    bool openAttachment(const MimeTreeParser::MessagePart::Ptr &message);
    bool importPublicKey(const MimeTreeParser::MessagePart::Ptr &message);

    Q_INVOKABLE QString saveAttachmentToPath(const int row, const QString &path);
    [[nodiscard]] QString saveAttachmentToPath(const MimeTreeParser::MessagePart::Ptr &part, const QString &path);

Q_SIGNALS:
    void info(const QString &message);
    void errorOccurred(const QString &message);

private:
    std::unique_ptr<AttachmentModelPrivate> d;
};
