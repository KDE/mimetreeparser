// SPDX-FileCopyrightText: 2023 g10 Code GmbH
// SPDX-FileContributor: Carl Schwan <carl.schwan@gnupg.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "fileopener.h"
#include "mimetreeparser_core_debug.h"

#include <KMbox/MBox>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>

using namespace MimeTreeParser::Core;

namespace
{
KMime::Message::Ptr openSmimeEncrypted(const QByteArray &content)
{
    KMime::Message::Ptr message(new KMime::Message);

    auto contentType = message->contentType();
    contentType->setMimeType(QByteArrayLiteral("application/pkcs7-mime"));
    contentType->setParameter(QByteArrayLiteral("smime-type"), QStringLiteral("enveloped-data"));

    auto contentDisposition = new KMime::Headers::ContentDisposition;
    contentDisposition->setDisposition(KMime::Headers::CDattachment);
    contentDisposition->setFilename(QStringLiteral("smime.p7m"));
    message->appendHeader(contentDisposition);

    auto cte = message->contentTransferEncoding();
    cte->setEncoding(KMime::Headers::CE7Bit);

    message->setBody(content);
    message->assemble();

    return message;
}

KMime::Message::Ptr openPgpEncrypted(const QByteArray &content)
{
    KMime::Message::Ptr message(new KMime::Message);

    auto contentType = message->contentType();
    contentType->setMimeType(QByteArrayLiteral("multipart/encrypted"));
    contentType->setBoundary(KMime::multiPartBoundary());
    contentType->setParameter(QByteArrayLiteral("protocol"), QStringLiteral("application/pgp-encrypted"));

    auto cte = message->contentTransferEncoding();
    cte->setEncoding(KMime::Headers::CE7Bit);

    auto pgpEncrypted = new KMime::Content;
    pgpEncrypted->contentType()->setMimeType(QByteArrayLiteral("application/pgp-encrypted"));
    auto contentDisposition = new KMime::Headers::ContentDisposition;
    contentDisposition->setDisposition(KMime::Headers::CDattachment);
    pgpEncrypted->appendHeader(contentDisposition);
    pgpEncrypted->setBody("Version: 1");
    message->appendContent(pgpEncrypted);

    auto encryptedContent = new KMime::Content;
    encryptedContent->contentType()->setMimeType(QByteArrayLiteral("application/octet-stream"));
    contentDisposition = new KMime::Headers::ContentDisposition;
    contentDisposition->setDisposition(KMime::Headers::CDinline);
    contentDisposition->setFilename(QStringLiteral("msg.asc"));
    encryptedContent->appendHeader(contentDisposition);
    encryptedContent->setBody(content);
    message->appendContent(encryptedContent);

    message->assemble();

    return message;
}

QList<KMime::Message::Ptr> openMbox(const QString &fileName)
{
    KMBox::MBox mbox;
    const bool ok = mbox.load(fileName);
    if (!ok) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "Unable to open" << fileName;
        return {};
    }

    QList<KMime::Message::Ptr> messages;
    const auto entries = mbox.entries();
    for (const auto &entry : entries) {
        messages << KMime::Message::Ptr(mbox.readMessage(entry));
    }
    return messages;
}
}

QList<KMime::Message::Ptr> FileOpener::openFile(const QString &fileName)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(fileName);

    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    if (!file.isOpen()) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "Could not open file";
        return {};
    }

    const auto content = file.readAll();

    if (content.isEmpty()) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "File is empty";
        return {};
    }

    if (mime.inherits(QStringLiteral("application/pkcs7-mime")) || fileName.endsWith(QStringLiteral("smime.p7m"))) {
        return {openSmimeEncrypted(content)};
    } else if (mime.inherits(QStringLiteral("application/pgp-encrypted")) || fileName.endsWith(QStringLiteral(".asc"))) {
        return {openPgpEncrypted(content)};
    } else if (content.startsWith("From ")) {
        return openMbox(fileName);
    } else {
        auto msg = new KMime::Message;
        msg->setContent(KMime::CRLFtoLF(content));
        msg->parse();
        if (!msg->hasContent()) {
            delete msg;
            msg = nullptr;
            return {};
        }
        return {KMime::Message::Ptr(msg)};
    }

    return {};
}
