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

    [[nodiscard]] QString structureAsString() const;
    void print();

    /// The text of the message, ie. what would appear in the
    /// composer's text editor if this was edited or replied to.
    /// This is usually the content of the first text/plain MIME part.
    [[nodiscard]] QString plainTextContent();

    /// Similar to plainTextContent(), but returns the HTML source of
    /// the first text/html MIME part.
    [[nodiscard]] QString htmlContent();

    /// Returns whether the parsed message contains encrypted parts.
    [[nodiscard]] bool hasEncryptedParts() const;

    /// Returns whether the parsed message contains signed parts.
    ///
    /// \warning This doesn't check whether signed parts exists inside
    /// encrypted parts that have not been decrypted yet.
    [[nodiscard]] bool hasSignedParts() const;

    /** Parse beginning at a given node and recursively parsing
      the children of that node and it's next sibling. */
    void parseObjectTree(KMime::Content *node);
    void parseObjectTree(const QByteArray &mimeMessage);
    MessagePart::Ptr parsedPart() const;
    [[nodiscard]] KMime::Content *find(const std::function<bool(KMime::Content *)> &select);
    [[nodiscard]] MessagePart::List collectContentParts();
    [[nodiscard]] MessagePart::List collectContentParts(MessagePart::Ptr start);
    [[nodiscard]] MessagePart::List collectAttachmentParts();

    /** Decrypt parts and verify signatures */
    void decryptAndVerify();

    /** Embedd content referenced by cid by inlining */
    [[nodiscard]] QString resolveCidLinks(const QString &html);

private:
    /**
     * Does the actual work for parseObjectTree. Unlike parseObjectTree(), this does not change the
     * top-level content.
     */
    MIMETREEPARSER_CORE_NO_EXPORT MessagePart::Ptr parseObjectTreeInternal(KMime::Content *node, bool mOnlyOneMimePart);
    MIMETREEPARSER_CORE_NO_EXPORT MessagePart::List processType(KMime::Content *node, const QByteArray &mediaType, const QByteArray &subType);

    MIMETREEPARSER_CORE_NO_EXPORT MessagePart::List defaultHandling(KMime::Content *node);

    MIMETREEPARSER_CORE_NO_EXPORT QByteArray codecNameFor(KMime::Content *node) const;

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
