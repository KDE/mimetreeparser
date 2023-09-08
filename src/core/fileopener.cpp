// SPDX-FileCopyrigthText: 2023 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "fileopener.h"
#include "mimetreeparser_core_debug.h"

#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>

using namespace MimeTreeParser::Core;

QVector<KMime::Message::Ptr> FileOpener::openFile(const QString &fileName)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(fileName);
    KMime::Message::Ptr message(new KMime::Message);

    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    if (!file.isOpen()) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "Could not open file";
        return {};
    }

    const auto content = file.readAll();

    if (content.length() == 0) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << "File is empty";
        return {};
    }

    if (mime.inherits(QStringLiteral("application/pkcs7-mime")) || fileName.endsWith(QStringLiteral("smime.p7m"))) {
        auto contentType = message->contentType();
        contentType->setMimeType("application/pkcs7-mime");
        contentType->setParameter(QStringLiteral("smime-type"), QStringLiteral("enveloped-data"));

        auto contentDisposition = new KMime::Headers::ContentDisposition;
        contentDisposition->setDisposition(KMime::Headers::CDattachment);
        contentDisposition->setFilename(QStringLiteral("smime.p7m"));
        message->appendHeader(contentDisposition);

        auto cte = message->contentTransferEncoding();
        cte->setEncoding(KMime::Headers::CE7Bit);
        cte->setDecoded(true);

        message->setBody(content);
        message->assemble();
    } else if (mime.inherits(QStringLiteral("application/pgp-encrypted")) || fileName.endsWith(QStringLiteral(".asc"))) {
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
        int startOfMessage = 0;
        if (content.startsWith("From ")) {
            startOfMessage = content.indexOf('\n');
            if (startOfMessage == -1) {
                return {};
            }
            startOfMessage += 1; // the message starts after the '\n'
        }
        QVector<KMime::Message::Ptr> listMessages;

        // check for multiple messages in the file
        int endOfMessage = content.indexOf("\nFrom ", startOfMessage);
        while (endOfMessage != -1) {
            if (content.indexOf("From ", startOfMessage) == startOfMessage) {
                startOfMessage = content.indexOf('\n', startOfMessage) + 1;
            }
            auto msg = new KMime::Message;
            msg->setContent(KMime::CRLFtoLF(content.mid(startOfMessage, endOfMessage - startOfMessage)));
            msg->parse();
            if (!msg->hasContent()) {
                delete msg;
                msg = nullptr;
                return {};
            }
            KMime::Message::Ptr mMsg(msg);
            listMessages << mMsg;
            startOfMessage = endOfMessage + 1;
            endOfMessage = content.indexOf("\nFrom ", startOfMessage);
        }
        if (endOfMessage == -1) {
            if (content.indexOf("From ", startOfMessage) == startOfMessage) {
                startOfMessage = content.indexOf('\n', startOfMessage) + 1;
            }
            endOfMessage = content.length();
            auto msg = new KMime::Message;
            msg->setContent(KMime::CRLFtoLF(content.mid(startOfMessage, endOfMessage - startOfMessage)));
            msg->parse();
            if (!msg->hasContent()) {
                delete msg;
                msg = nullptr;
                return {};
            }
            KMime::Message::Ptr mMsg(msg);
            listMessages << mMsg;
        }
        return listMessages;
    }

    return {message};
}
