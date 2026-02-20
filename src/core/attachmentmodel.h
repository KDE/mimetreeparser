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
/*!
 * \class MimeTreeParser::AttachmentModel
 * \inmodule MimeTreeParserCore
 * \inheaderfile MimeTreeParserCore/AttachmentModel
 */
class MIMETREEPARSER_CORE_EXPORT AttachmentModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    /*!
     * \brief AttachmentModel
     * \param parser
     */
    explicit AttachmentModel(std::shared_ptr<MimeTreeParser::ObjectTreeParser> parser);
    /*!
     */
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

    /*!
     * \brief Returns the role names for the model
     */
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    /*!
     * \brief Returns the data for the given index and role
     * \param index The model index
     * \param role The data role
     */
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    /*!
     * \brief Returns the header data for the given section and role
     * \param section The section index
     * \param orientation The orientation of the header
     * \param role The data role
     */
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    /*!
     * \brief Returns the number of rows in the model
     * \param parent The parent index
     */
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    /*!
     * \brief Returns the number of columns in the model
     * \param parent The parent index
     */
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /*!
     * \brief Opens the attachment at the given row
     * \param row The row index of the attachment
     */
    Q_INVOKABLE bool openAttachment(const int row);
    /*!
     * \brief Imports the public key from the attachment at the given row
     * \param row The row index of the attachment
     */
    Q_INVOKABLE bool importPublicKey(const int row);

    /*!
     * \brief Opens the attachment from the message part
     * \param message The message part containing the attachment
     */
    bool openAttachment(const QSharedPointer<MimeTreeParser::MessagePart> &message);
    /*!
     * \brief Imports the public key from the message part
     * \param message The message part containing the public key
     */
    bool importPublicKey(const QSharedPointer<MimeTreeParser::MessagePart> &message);

    /*!
     * \brief Saves the attachment at the given row to the specified path
     * \param row The row index of the attachment
     * \param path The destination path
     */
    Q_INVOKABLE QString saveAttachmentToPath(const int row, const QString &path);
    /*!
     * \brief Saves the attachment from the message part to the specified path
     * \param part The message part containing the attachment
     * \param path The destination path
     */
    QString saveAttachmentToPath(const QSharedPointer<MimeTreeParser::MessagePart> &part, const QString &path);

Q_SIGNALS:
    /*!
     * \brief Emitted to provide informational messages
     * \param message The information message
     */
    void info(const QString &message);
    /*!
     * \brief Emitted when an error occurs
     * \param message The error message
     */
    void errorOccurred(const QString &message);

private:
    std::unique_ptr<AttachmentModelPrivate> d;
};
