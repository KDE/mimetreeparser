// SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "fileopener.h"
#include <KMbox/MBox>
#include <MimeTreeParser/ObjectTreeParser>
#include <QDebug>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>

void FileOpener::open(const QUrl &url)
{
    const auto fileName = url.fileName();
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(fileName);

    if (mime.inherits(QStringLiteral("application/mbox"))) {
        KMBox::MBox mbox;
        mbox.load(url.toLocalFile());
        const auto entries = mbox.entries();
        if (!entries.isEmpty()) {
            KMime::Message::Ptr message(mbox.readMessage(entries[0]));
            Q_EMIT messageOpened(message);
            return;
        }
    }

    QFile file(url.toLocalFile());
    file.open(QIODevice::ReadOnly);
    if (!file.isOpen()) {
        qWarning() << "Could not open file";
        return;
    }

    const auto content = file.readAll();

    if (content.length() == 0) {
        qWarning() << "File is empty";
        return;
    }

    KMime::Message::Ptr message(new KMime::Message);

    if (mime.inherits(QStringLiteral("application/pgp-encrypted")) || fileName.endsWith(QStringLiteral(".asc"))) {
        auto contentType = message->contentType();
        contentType->setMimeType("multipart/encrypted");
        contentType->setBoundary(KMime::multiPartBoundary());
        contentType->setParameter(QStringLiteral("protocol"), QStringLiteral("application/pgp-encrypted"));
        contentType->setCategory(KMime::Headers::CCcontainer);

        auto cte = message->contentTransferEncoding();
        cte->setEncoding(KMime::Headers::CE7Bit);
        cte->setDecoded(true);

        auto pgpEncrypted = new KMime::Content;
        pgpEncrypted->contentType()->setMimeType("application/pgp-encrypted");
        auto contentDisposition = new KMime::Headers::ContentDisposition;
        contentDisposition->setDisposition(KMime::Headers::CDattachment);
        pgpEncrypted->appendHeader(contentDisposition);
        pgpEncrypted->setBody("Version: 1");
        message->addContent(pgpEncrypted);

        auto encryptedContent = new KMime::Content;
        encryptedContent->contentType()->setMimeType("application/octet-stream");
        contentDisposition = new KMime::Headers::ContentDisposition;
        contentDisposition->setDisposition(KMime::Headers::CDinline);
        contentDisposition->setFilename(QStringLiteral("msg.asc"));
        encryptedContent->appendHeader(contentDisposition);
        encryptedContent->setBody(content);
        message->addContent(encryptedContent);

        message->assemble();
    } else {
        message->setContent(content);
        message->parse();
    }

    Q_EMIT messageOpened(message);
}
