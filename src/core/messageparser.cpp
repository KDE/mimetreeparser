// SPDX-FileCopyrightText: 2016 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "messageparser.h"

#include "objecttreeparser.h"
#include <KLocalizedString>
#include <QElapsedTimer>

#include "async.h"
#include "attachmentmodel.h"
#include "partmodel.h"

namespace
{

template<typename T>
const T *findHeader(KMime::Content *content)
{
    auto header = content->header<T>();
    if (header || !content->parent()) {
        return header;
    }
    return findHeader<T>(content->parent());
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
        qWarning() << Q_FUNC_INFO << "Empty message given";
        return;
    }
    d->mMessage = message;

    QElapsedTimer time;
    time.start();
    auto parser = std::make_shared<MimeTreeParser::ObjectTreeParser>();
    parser->parseObjectTree(message.data());
    qDebug() << "Message parsing took: " << time.elapsed();
    parser->decryptParts();
    qDebug() << "Message parsing and decryption/verification: " << time.elapsed();
    d->mParser = parser;
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
    return new AttachmentModel(d->mParser);
}

QString MessageParser::subject() const
{
    if (d->mMessage) {
        return findHeader<KMime::Headers::Subject>(d->mMessage.get())->asUnicodeString();
    } else {
        return QString();
    }
}

QString MessageParser::from() const
{
    if (d->mMessage) {
        return findHeader<KMime::Headers::From>(d->mMessage.get())->displayString();
    } else {
        return QString();
    }
}

QString MessageParser::sender() const
{
    if (d->mMessage) {
        return findHeader<KMime::Headers::Sender>(d->mMessage.get())->displayString();
    } else {
        return QString();
    }
}
QString MessageParser::to() const
{
    if (d->mMessage) {
        return findHeader<KMime::Headers::To>(d->mMessage.get())->displayString();
    } else {
        return i18nc("displayed when a mail has unknown sender, receiver or date", "Unknown");
    }
}

QDateTime MessageParser::date() const
{
    if (d->mMessage) {
        return findHeader<KMime::Headers::Date>(d->mMessage.get())->dateTime();
    } else {
        return QDateTime();
    }
}
