// SPDX-FileCopyrightText: 2016 Sandro Knauß <knauss@kolabsys.com>
// SPDX-FileCopyCopyright: 2017 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "attachmentmodel.h"

#include "mimetreeparser_core_debug.h"

#include <QGpgME/ImportJob>
#include <QGpgME/Protocol>

#include <KLocalizedString>
#include <KMime/Content>

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QMimeDatabase>
#include <QMimeType>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>

#ifdef Q_OS_WIN
#include <cstdio>
#include <string>
#include <vector>
#include <windows.h>
#endif

namespace
{

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

// SPDX-SnippetBegin
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: GPL-3.0-only

#define WINDOWS_DEVICES_PATTERN "(CON|AUX|PRN|NUL|COM[1-9]|LPT[1-9])(\\..*)?"

// Naming a file like a device name will break on Windows, even if it is
// "com1.txt". Since we are cross-platform, we generally disallow such file
//  names.
const QRegularExpression &windowsDeviceNoSubDirPattern()
{
    static const QRegularExpression rc(QStringLiteral("^" WINDOWS_DEVICES_PATTERN "$"), QRegularExpression::CaseInsensitiveOption);
    Q_ASSERT(rc.isValid());
    return rc;
}

const QRegularExpression &windowsDeviceSubDirPattern()
{
    static const QRegularExpression rc(QStringLiteral("^.*[/\\\\]" WINDOWS_DEVICES_PATTERN "$"), QRegularExpression::CaseInsensitiveOption);
    Q_ASSERT(rc.isValid());
    return rc;
}

/* Validate a file base name, check for forbidden characters/strings. */

#define SLASHES "/\\"

static const char notAllowedCharsSubDir[] = ",^@={}[]~!?:&*\"|#%<>$\"'();`' ";
static const char notAllowedCharsNoSubDir[] = ",^@={}[]~!?:&*\"|#%<>$\"'();`' " SLASHES;

static const char *notAllowedSubStrings[] = {".."};

bool validateFileName(const QString &name, bool allowDirectories)
{
    if (name.isEmpty()) {
        return false;
    }

    // Characters
    const char *notAllowedChars = allowDirectories ? notAllowedCharsSubDir : notAllowedCharsNoSubDir;
    for (const char *c = notAllowedChars; *c; c++) {
        if (name.contains(QLatin1Char(*c))) {
            return false;
        }
    }

    // Substrings
    const int notAllowedSubStringCount = sizeof(notAllowedSubStrings) / sizeof(const char *);
    for (int s = 0; s < notAllowedSubStringCount; s++) {
        const QLatin1StringView notAllowedSubString(notAllowedSubStrings[s]);
        if (name.contains(notAllowedSubString)) {
            return false;
        }
    }

    // Windows devices
    bool matchesWinDevice = name.contains(windowsDeviceNoSubDirPattern());
    if (!matchesWinDevice && allowDirectories) {
        matchesWinDevice = name.contains(windowsDeviceSubDirPattern());
    }
    return !matchesWinDevice;
}
// SPDX-SnippetEnd
}

#ifdef Q_OS_WIN
struct WindowFile {
    std::wstring fileName;
    std::wstring dirName;
    HANDLE handle;
};
#endif

class AttachmentModelPrivate
{
public:
    AttachmentModelPrivate(AttachmentModel *q_ptr, const std::shared_ptr<MimeTreeParser::ObjectTreeParser> &parser);

    AttachmentModel *q;
    QMimeDatabase mimeDb;
    std::shared_ptr<MimeTreeParser::ObjectTreeParser> mParser;
    MimeTreeParser::MessagePart::List mAttachments;

#ifdef Q_OS_WIN
    std::vector<WindowFile> mOpenFiles;
#endif
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
#ifdef Q_OS_WIN
    for (const auto &file : d->mOpenFiles) {
        // As owner of the file we need to close our handle first
        // With FILE_SHARE_DELETE we have ensured that all _other_ processes must
        // have opened the file with FILE_SHARE_DELETE, too.
        if (!CloseHandle(file.handle)) {
            // Always get the last error before calling any Qt functions that may
            // use Windows system calls.
            DWORD err = GetLastError();
            qWarning() << "Unable to close handle for file" << QString::fromStdWString(file.fileName) << err;
        }

        if (!DeleteFileW(file.fileName.c_str())) {
            DWORD err = GetLastError();
            qWarning() << "Unable to delete file" << QString::fromStdWString(file.fileName) << err;
        }

        if (!RemoveDirectoryW(file.dirName.c_str())) {
            DWORD err = GetLastError();
            qWarning() << "Unable to delete temporary directory" << QString::fromStdWString(file.dirName) << err;
        }
    }
#endif
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
            return i18ndc("mimetreeparser", "@title:column", "Name");
        case SizeColumn:
            return i18ndc("mimetreeparser", "@title:column", "Size");
        case IsEncryptedColumn:
            return i18ndc("mimetreeparser", "@title:column", "Encrypted");
        case IsSignedColumn:
            return i18ndc("mimetreeparser", "@title:column", "Signed");
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
    const auto mimetype = d->mimeDb.mimeTypeForName(QString::fromLatin1(part->mimeType()));
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

QString AttachmentModel::saveAttachmentToPath(const int row, const QString &path)
{
    const auto part = d->mAttachments.at(row);
    return saveAttachmentToPath(part, path);
}

QString AttachmentModel::saveAttachmentToPath(const MimeTreeParser::MessagePart::Ptr &part, const QString &path)
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

    QFile f(path);
    if (!f.open(QIODevice::ReadWrite)) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "Failed to write attachment to file:" << path << " Error: " << f.errorString();
        Q_EMIT errorOccurred(i18ndc("mimetreeparser", "@info", "Failed to save attachment."));
        return {};
    }
    f.write(data);
    f.close();
    qCInfo(MIMETREEPARSER_CORE_LOG) << "Wrote attachment to file: " << path;
    return path;
}

bool AttachmentModel::openAttachment(const int row)
{
    const auto part = d->mAttachments.at(row);
    return openAttachment(part);
}

bool AttachmentModel::openAttachment(const MimeTreeParser::MessagePart::Ptr &message)
{
    QString fileName = message->filename();
    QTemporaryDir tempDir(QDir::tempPath() + QLatin1Char('/') + qGuiApp->applicationName() + QStringLiteral(".XXXXXX"));
    // TODO: We need some cleanup here. Otherwise the files will stay forever on Windows.
    tempDir.setAutoRemove(false);
    if (message->filename().isEmpty() || !validateFileName(fileName, false)) {
        const auto mimetype = d->mimeDb.mimeTypeForName(QString::fromLatin1(message->mimeType()));
        fileName = tempDir.filePath(i18n("attachment") + QLatin1Char('.') + mimetype.preferredSuffix());
    } else {
        fileName = tempDir.filePath(message->filename());
    }

    const auto filePath = saveAttachmentToPath(message, fileName);
    if (filePath.isEmpty()) {
        Q_EMIT errorOccurred(i18ndc("mimetreeparser", "@info", "Failed to write attachment for opening."));
        return false;
    }

#ifdef Q_OS_WIN
    std::wstring fileNameStr = filePath.toStdWString();

    HANDLE hFile = CreateFileW(fileNameStr.c_str(),
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_DELETE, // allow other processes to delete it
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, // Using FILE_FLAG_DELETE_ON_CLOSE causes some
                                                      // applications like windows zip not to open the
                                                      // file.
                               NULL // no template
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        Q_EMIT errorOccurred(i18ndc("mimetreeparser", "@info", "Failed to open attachment."));
        QFile file(fileName);
        file.remove();
        return false;
    }

    d->mOpenFiles.push_back({fileNameStr, tempDir.path().toStdWString(), hFile});
#endif

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) {
        Q_EMIT errorOccurred(i18ndc("mimetreeparser", "@info", "Failed to open attachment."));
        return false;
    }

    return true;
}

bool AttachmentModel::importPublicKey(const int row)
{
    const auto part = d->mAttachments.at(row);
    return importPublicKey(part);
}

bool AttachmentModel::importPublicKey(const MimeTreeParser::MessagePart::Ptr &part)
{
    Q_ASSERT(part);
    const QByteArray certData = part->node()->decodedContent();
    QGpgME::ImportJob *importJob = QGpgME::openpgp()->importJob();

    connect(importJob, &QGpgME::AbstractImportJob::result, this, [this](const GpgME::ImportResult &result) {
        if (result.numConsidered() == 0) {
            Q_EMIT errorOccurred(i18ndc("mimetreeparser", "@info", "No keys were found in this attachment"));
            return;
        } else {
            QString message = i18ndcp("mimetreeparser", "@info", "one key imported", "%1 keys imported", result.numImported());
            if (result.numUnchanged() != 0) {
                message += QStringLiteral("\n")
                    + i18ndcp("mimetreeparser", "@info", "one key was already imported", "%1 keys were already imported", result.numUnchanged());
            }
            Q_EMIT info(message);
        }
    });
    GpgME::Error err = importJob->start(certData);
    return !err;
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
