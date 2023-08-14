// SPDX-FileCopyrightText: 2016 Sandro Knauß <sknauss@kde.org>
// SPDX-FileCopyrightText: 2003 Marc Mutz <mutz@kde.org>
// SPDX-FileCopyrightText: 2002-2003, 2009 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
// SPDX-FileCopyrightText: 2009 Andras Mantia <andras@kdab.net>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "messagepart.h"

#include "mimetreeparser_core_export.h"
#include <QSharedPointer>
#include <functional>

class QString;

namespace KMime
{
class Content;
}

class QTextCodec;

namespace MimeTreeParser
{

/**
    Entry point to parse mime messages.

    Content returned by the ObjectTreeParser (including messageparts),
    is normalized to not contain any CRLF's but only LF's (just like KMime).
*/
class MIMETREEPARSER_CORE_EXPORT ObjectTreeParser
{
    // Disable copy
    ObjectTreeParser(const ObjectTreeParser &other);

public:
    explicit ObjectTreeParser() = default;
    virtual ~ObjectTreeParser() = default;

    QString structureAsString() const;
    void print();

    /**
     * The text of the message, ie. what would appear in the
     * composer's text editor if this was edited or replied to.
     * This is usually the content of the first text/plain MIME part.
     */
    QString plainTextContent();

    /**
     * Similar to plainTextContent(), but returns the HTML source of the first text/html MIME part.
     */
    QString htmlContent();

    /** Parse beginning at a given node and recursively parsing
      the children of that node and it's next sibling. */
    void parseObjectTree(KMime::Content *node);
    void parseObjectTree(const QByteArray &mimeMessage);
    MessagePart::Ptr parsedPart() const;
    KMime::Content *find(const std::function<bool(KMime::Content *)> &select);
    MessagePart::List collectContentParts();
    MessagePart::List collectContentParts(MessagePart::Ptr start);
    MessagePart::List collectAttachmentParts();

    /** Decrypt parts and verify signatures */
    void decryptAndVerify();
    // DEPRECATED calls decryptAndVerify
    void decryptParts();

    /** Import any certificates found in the message */
    void importCertificates();

    /** Embedd content referenced by cid by inlining */
    QString resolveCidLinks(const QString &html);

    KMime::Message::Ptr message() const;

private:
    /**
     * Does the actual work for parseObjectTree. Unlike parseObjectTree(), this does not change the
     * top-level content.
     */
    MessagePart::Ptr parseObjectTreeInternal(KMime::Content *node, bool mOnlyOneMimePart);
    MessagePart::List processType(KMime::Content *node, const QByteArray &mediaType, const QByteArray &subType);

    MessagePart::List defaultHandling(KMime::Content *node);

    const QTextCodec *codecFor(KMime::Content *node) const;

    KMime::Content *mTopLevelContent{nullptr};
    MessagePart::Ptr mParsedPart;

    KMime::Message::Ptr mMsg;

    friend class MessagePart;
    friend class EncryptedMessagePart;
    friend class SignedMessagePart;
    friend class EncapsulatedRfc822MessagePart;
    friend class TextMessagePart;
    friend class HtmlMessagePart;
    friend class TextPlainBodyPartFormatter;
    friend class MultiPartSignedBodyPartFormatter;
    friend class ApplicationPkcs7MimeBodyPartFormatter;
};

}
