// SPDX-FileCopyrightText: 2016 Sandro Knauß <knauss@kolabsys.com>
// SPDX-FileCopyCopyright: 2017 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "attachmentmodel.h"

#include <QString>

#include <KLocalizedString>
#include <KMime/Content>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QUrl>

#include <QStringLiteral>

QString sizeHuman(float size)
{
    QStringList list;
    list << QStringLiteral("KB") << QStringLiteral("MB") << QStringLiteral("GB") << QStringLiteral("TB");

    QStringListIterator i(list);
    QString unit = QStringLiteral("Bytes");

    while (size >= 1024.0 && i.hasNext()) {
        unit = i.next();
        size /= 1024.0;
    }

    if (unit == QStringLiteral("Bytes")) {
        return QString().setNum(size) + QStringLiteral(" ") + unit;
    } else {
        return QString().setNum(size, 'f', 2) + QStringLiteral(" ") + unit;
    }
}

class AttachmentModelPrivate
{
public:
    AttachmentModelPrivate(AttachmentModel *q_ptr, const std::shared_ptr<MimeTreeParser::ObjectTreeParser> &parser);

    AttachmentModel *q;
    std::shared_ptr<MimeTreeParser::ObjectTreeParser> mParser;
    MimeTreeParser::MessagePart::List mAttachments;
};

AttachmentModelPrivate::AttachmentModelPrivate(AttachmentModel *q_ptr, const std::shared_ptr<MimeTreeParser::ObjectTreeParser> &parser)
    : q(q_ptr)
    , mParser(parser)
{
    mAttachments = mParser->collectAttachmentParts();
}

AttachmentModel::AttachmentModel(std::shared_ptr<MimeTreeParser::ObjectTreeParser> parser)
    : QAbstractTableModel()
    , d(std::unique_ptr<AttachmentModelPrivate>(new AttachmentModelPrivate(this, parser)))
{
}

AttachmentModel::~AttachmentModel()
{
}

QHash<int, QByteArray> AttachmentModel::roleNames() const
{
    return {
        {TypeRole, QByteArrayLiteral("type")},
        {NameRole, QByteArrayLiteral("name")},
        {SizeRole, QByteArrayLiteral("size")},
        {IconRole, QByteArrayLiteral("iconName")},
        {IsEncryptedRole, QByteArrayLiteral("encrypted")},
        {IsSignedRole, QByteArrayLiteral("signed")},
    };
}

QVariant AttachmentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case NameColumn:
            return i18n("Name");
        case SizeColumn:
            return i18n("Size");
        case IsEncryptedColumn:
            return i18n("Encrypted");
        case IsSignedColumn:
            return i18n("Signed");
        }
    }
    return {};
}

QVariant AttachmentModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();
    const auto column = index.column();

    const auto part = d->mAttachments.at(row);
    Q_ASSERT(part);
    auto node = part->node();
    if (!node) {
        qWarning() << "no content for attachment";
        return {};
    }
    QMimeDatabase mimeDb;
    const auto mimetype = mimeDb.mimeTypeForName(QString::fromLatin1(part->mimeType()));
    const auto content = node->encodedContent();

    switch (column) {
    case NameColumn:
        switch (role) {
        case TypeRole:
            return mimetype.name();
        case Qt::DisplayRole:
        case NameRole:
            return part->filename();
        case IconRole:
            return mimetype.iconName();
        case Qt::DecorationRole:
            return QIcon::fromTheme(mimetype.iconName());
        case SizeRole:
            return sizeHuman(content.size());
        case IsEncryptedRole:
            return part->encryptions().size() > 0;
        case IsSignedRole:
            return part->signatures().size() > 0;
        case AttachmentPartRole:
            return QVariant::fromValue(part);
        default:
            return {};
        }
    case SizeColumn:
        switch (role) {
        case Qt::DisplayRole:
            return sizeHuman(content.size());
        default:
            return {};
        }
    case IsEncryptedColumn:
        switch (role) {
        case Qt::CheckStateRole:
            return part->encryptions().size() > 0 ? Qt::Checked : Qt::Unchecked;
        default:
            return {};
        }
    case IsSignedColumn:
        switch (role) {
        case Qt::CheckStateRole:
            return part->signatures().size() > 0 ? Qt::Checked : Qt::Unchecked;
        default:
            return {};
        }
    default:
        return {};
    }
}

static QString internalSaveAttachmentToDisk(AttachmentModel *model, const MimeTreeParser::MessagePart::Ptr &part, const QString &path, bool readonly = false)
{
    Q_ASSERT(part);
    auto node = part->node();
    auto data = node->decodedContent();
    // This is necessary to store messages embedded messages (EncapsulatedRfc822MessagePart)
    if (data.isEmpty()) {
        data = node->encodedContent();
    }
    if (part->isText()) {
        // convert CRLF to LF before writing text attachments to disk
        data = KMime::CRLFtoLF(data);
    }
    const auto name = part->filename();
    QString fname = path + name;

    // Fallback name should we end up with an empty name
    if (name.isEmpty()) {
        fname = path + QStringLiteral("unnamed");
        while (QFileInfo::exists(fname)) {
            fname = fname + QStringLiteral("_1");
        }
    }

    // A file with that name already exists, we assume it's the right file
    if (QFileInfo::exists(fname)) {
        return fname;
    }
    QFile f(fname);
    if (!f.open(QIODevice::ReadWrite)) {
        qWarning() << "Failed to write attachment to file:" << fname << " Error: " << f.errorString();
        Q_EMIT model->info(i18n("Failed to save attachment."));
        return {};
    }
    f.write(data);
    if (readonly) {
        // make file read-only so that nobody gets the impression that he migh edit attached files
        f.setPermissions(QFileDevice::ReadUser);
    }
    f.close();
    qInfo() << "Wrote attachment to file: " << fname;
    return fname;
}

bool AttachmentModel::saveAttachmentToDisk(const int row)
{
    const auto part = d->mAttachments.at(row);
    return saveAttachmentToDisk(part);
}

bool AttachmentModel::saveAttachmentToDisk(const MimeTreeParser::MessagePart::Ptr &message)
{
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (downloadDir.isEmpty()) {
        downloadDir = QStringLiteral("~");
    }
    downloadDir += QLatin1Char('/') + qGuiApp->applicationName();
    QDir{}.mkpath(downloadDir);

    auto path = internalSaveAttachmentToDisk(this, message, downloadDir);
    if (path.isEmpty()) {
        return false;
    }
    Q_EMIT info(i18n("Saved the attachment to disk: %1", path));
    return true;
}

bool AttachmentModel::openAttachment(const int row)
{
    const auto part = d->mAttachments.at(row);
    return openAttachment(part);
}

bool AttachmentModel::openAttachment(const MimeTreeParser::MessagePart::Ptr &message)
{
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1Char('/') + qGuiApp->applicationName();
    QDir{}.mkpath(downloadDir);
    const auto filePath = internalSaveAttachmentToDisk(this, message, downloadDir, true);
    if (!filePath.isEmpty()) {
        if (!QDesktopServices::openUrl(QUrl(QStringLiteral("file://") + filePath))) {
            Q_EMIT info(i18n("Failed to open attachment."));
            return false;
        }
        return true;
    }
    Q_EMIT info(i18n("Failed to save attachment for opening."));
    return false;
}

bool AttachmentModel::importPublicKey(const int row)
{
    const auto part = d->mAttachments.at(row);
    return importPublicKey(part);
}

bool AttachmentModel::importPublicKey(const MimeTreeParser::MessagePart::Ptr &part)
{
    Q_ASSERT(part);
    auto result = Crypto::importKey(Crypto::OpenPGP, part->node()->decodedContent());

    bool success = true;
    QString message;
    if (result.considered == 0) {
        message = i18n("No keys were found in this attachment");
        success = false;
    } else {
        message = i18np("one key imported", "%1 keys imported", result.imported);
        if (result.unchanged != 0) {
            message += QStringLiteral("\n") + i18np("one key was already imported", "%1 keys were already imported", result.unchanged);
        }
    }

    Q_EMIT info(message);

    return success;
}

int AttachmentModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return d->mAttachments.size();
    }
    return 0;
}

int AttachmentModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return ColumnCount;
    }
    return 0;
}

#include "moc_attachmentmodel.cpp"
