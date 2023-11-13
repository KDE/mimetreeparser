// SPDX-FileCopyrightText: 2016 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "messageparser.h"

#include "attachmentmodel.h"
#include "mimetreeparser_core_debug.h"
#include "objecttreeparser.h"
#include "partmodel.h"

#include <KLocalizedString>

#include <QElapsedTimer>

namespace
{

template<typename T>
const T *findHeader(KMime::Content *content, KMime::Content *protectedHeaderNode)
{
    if (protectedHeaderNode) {
        auto header = protectedHeaderNode->header<T>();
        if (header) {
            return header;
        }
    }

    auto header = content->header<T>();
    if (header || !content->parent()) {
        return header;
    }
    return findHeader<T>(content->parent(), nullptr);
}

const KMime::Headers::Base *findHeader(KMime::Content *content, const char *headerType)
{
    const auto header = content->headerByType(headerType);
    if (header || !content->parent()) {
        return header;
    }
    return findHeader(content->parent(), headerType);
}
}

class MessagePartPrivate
{
public:
    std::shared_ptr<MimeTreeParser::ObjectTreeParser> mParser;
    KMime::Message::Ptr mMessage;
    KMime::Content *protectedHeaderNode = nullptr;
    std::unique_ptr<KMime::Content> ownedContent;
};

MessageParser::MessageParser(QObject *parent)
    : QObject(parent)
    , d(std::unique_ptr<MessagePartPrivate>(new MessagePartPrivate))
{
}

MessageParser::~MessageParser()
{
}

KMime::Message::Ptr MessageParser::message() const
{
    return d->mMessage;
}

void MessageParser::setMessage(const KMime::Message::Ptr message)
{
    if (message == d->mMessage) {
        return;
    }
    if (!message) {
        qCWarning(MIMETREEPARSER_CORE_LOG) << Q_FUNC_INFO << "Empty message given";
        return;
    }
    d->mMessage = message;

    QElapsedTimer time;
    time.start();
    auto parser = std::make_shared<MimeTreeParser::ObjectTreeParser>();
    parser->parseObjectTree(message.data());
    qCDebug(MIMETREEPARSER_CORE_LOG) << "Message parsing took: " << time.elapsed();
    parser->decryptParts();
    qCDebug(MIMETREEPARSER_CORE_LOG) << "Message parsing and decryption/verification: " << time.elapsed();
    d->mParser = parser;
    const auto contentParts = parser->collectContentParts();
    for (const auto &part : contentParts) {
        const auto contentType = part->node()->contentType();
        if (contentType && contentType->hasParameter(QStringLiteral("protected-headers"))) {
            const auto contentDisposition = part->node()->contentDisposition();

            // Check for legacy format for protected-headers
            if (contentDisposition && contentDisposition->disposition() == KMime::Headers::CDinline) {
                d->ownedContent = std::make_unique<KMime::Content>();
                // we put the decoded content as encoded content of the new node
                // as the header are inline in part->node()
                d->ownedContent->setContent(part->node()->decodedContent());
                d->ownedContent->parse();
                d->protectedHeaderNode = d->ownedContent.get();
                break;
            }
            d->protectedHeaderNode = part->node();
            break;
        }
    }

    Q_EMIT htmlChanged();
}

bool MessageParser::loaded() const
{
    return bool{d->mParser};
}

QString MessageParser::structureAsString() const
{
    if (!d->mParser) {
        return {};
    }
    return d->mParser->structureAsString();
}

PartModel *MessageParser::parts() const
{
    if (!d->mParser) {
        return nullptr;
    }
    const auto model = new PartModel(d->mParser);
    return model;
}

AttachmentModel *MessageParser::attachments() const
{
    if (!d->mParser) {
        return nullptr;
    }
    auto attachmentModel = new AttachmentModel(d->mParser);
    attachmentModel->setParent(const_cast<MessageParser *>(this));
    return attachmentModel;
}

QString MessageParser::subject() const
{
    if (d->mMessage) {
        const auto header = findHeader<KMime::Headers::Subject>(d->mMessage.get(), d->protectedHeaderNode);
        if (header) {
            return header->asUnicodeString();
        }
    }

    return QString();
}

QString MessageParser::from() const
{
    if (d->mMessage) {
        const auto header = findHeader<KMime::Headers::From>(d->mMessage.get(), d->protectedHeaderNode);
        if (header) {
            return header->displayString();
        }
    }
    return QString();
}

QString MessageParser::sender() const
{
    if (d->mMessage) {
        const auto header = findHeader<KMime::Headers::Sender>(d->mMessage.get(), d->protectedHeaderNode);
        if (header) {
            return header->displayString();
        }
    }

    return QString();
}

QString MessageParser::to() const
{
    if (d->mMessage) {
        const auto header = findHeader<KMime::Headers::To>(d->mMessage.get(), d->protectedHeaderNode);
        if (!header) {
            return {};
        }
        return header->displayString();
    }
    return QString();
}

QString MessageParser::cc() const
{
    if (d->mMessage) {
        const auto header = findHeader<KMime::Headers::Cc>(d->mMessage.get(), d->protectedHeaderNode);
        if (!header) {
            return {};
        }
        return header->displayString();
    }
    return QString();
}

QString MessageParser::bcc() const
{
    if (d->mMessage) {
        const auto header = findHeader<KMime::Headers::Bcc>(d->mMessage.get(), d->protectedHeaderNode);
        if (!header) {
            return {};
        }
        return header->displayString();
    }
    return QString();
}

QDateTime MessageParser::date() const
{
    if (d->mMessage) {
        const auto header = findHeader<KMime::Headers::Date>(d->mMessage.get(), d->protectedHeaderNode);
        if (header) {
            return header->dateTime();
        }
    }
    return QDateTime();
}
