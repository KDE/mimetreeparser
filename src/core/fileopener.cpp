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
using namespace Qt::Literals::StringLiterals;
namespace
{
QSharedPointer<KMime::Message> openSmimeEncrypted(const QByteArray &content)
{
    QSharedPointer<KMime::Message> message(new KMime::Message);

    auto contentType = message->contentType();
    contentType->setMimeType("application/pkcs7-mime"_ba);
    contentType->setParameter("smime-type"_ba, u"enveloped-data"_s);

    auto contentDisposition = new KMime::Headers::ContentDisposition;
    contentDisposition->setDisposition(KMime::Headers::CDattachment);
    contentDisposition->setFilename(u"smime.p7m"_s);
    message->appendHeader(contentDisposition);

    auto cte = message->contentTransferEncoding();
    cte->setEncoding(KMime::Headers::CE7Bit);

    message->setBody(content);
    message->assemble();

    return message;
}

QSharedPointer<KMime::Message> openPgpEncrypted(const QByteArray &content)
{
    QSharedPointer<KMime::Message> message(new KMime::Message);

    auto contentType = message->contentType();
    contentType->setMimeType("multipart/encrypted"_ba);
    contentType->setBoundary(KMime::multiPartBoundary());
    contentType->setParameter("protocol"_ba, u"application/pgp-encrypted"_s);

    auto cte = message->contentTransferEncoding();
    cte->setEncoding(KMime::Headers::CE7Bit);

    auto pgpEncrypted = new KMime::Content;
    pgpEncrypted->contentType()->setMimeType("application/pgp-encrypted"_ba);
    auto contentDisposition = new KMime::Headers::ContentDisposition;
    contentDisposition->setDisposition(KMime::Headers::CDattachment);
    pgpEncrypted->appendHeader(contentDisposition);
    pgpEncrypted->setBody("Version: 1");
    message->appendContent(pgpEncrypted);

    auto encryptedContent = new KMime::Content;
    encryptedContent->contentType()->setMimeType("application/octet-stream"_ba);
    contentDisposition = new KMime::Headers::ContentDisposition;
    contentDisposition->setDisposition(KMime::Headers::CDinline);
    contentDisposition->setFilename(u"msg.asc"_s);
    encryptedContent->appendHeader(contentDisposition);
    encryptedContent->setBody(content);
    message->appendContent(encryptedContent);

    message->assemble();

    return message;
}

QList<QSharedPointer<KMime::Message>> openMbox(const QString &fileName)
{
    KMBox::MBox mbox;
    const bool ok = mbox.load(fileName);
    if (!ok) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "Unable to open" << fileName;
        return {};
    }

    QList<QSharedPointer<KMime::Message>> messages;
    const auto entries = mbox.entries();
    for (const auto &entry : entries) {
        messages << QSharedPointer<KMime::Message>(mbox.readMessage(entry));
    }
    return messages;
}
}

QList<QSharedPointer<KMime::Message>> FileOpener::openFile(const QString &fileName)
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

    if (mime.inherits(u"application/pkcs7-mime"_s) || fileName.endsWith(u"smime.p7m"_s)) {
        return {openSmimeEncrypted(content)};
    } else if (mime.inherits(u"application/pgp-encrypted"_s) || fileName.endsWith(u".asc"_s)) {
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
        return {QSharedPointer<KMime::Message>(msg)};
    }

    return {};
}
