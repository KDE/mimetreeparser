// SPDX-FileCopyrightText: 2016 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "messageparser.h"

#include "objecttreeparser.h"
#include <KLocalizedString>
#include <QElapsedTimer>

#include "async.h"
#include "attachmentmodel.h"
#include "partmodel.h"

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

QAbstractItemModel *MessageParser::parts() const
{
    if (!d->mParser) {
        return nullptr;
    }
    const auto model = new PartModel(d->mParser);
    return model;
}

QAbstractItemModel *MessageParser::attachments() const
{
    if (!d->mParser) {
        return nullptr;
    }
    const auto model = new AttachmentModel(d->mParser);
    return model;
}

QString MessageParser::subject() const
{
    if (d->mMessage && d->mMessage->subject()) {
        return d->mMessage->subject()->asUnicodeString();
    } else {
        return i18nc("displayed as subject when the subject of a mail is empty", "No Subject");
    }
}

QString MessageParser::from() const
{
    if (d->mMessage && d->mMessage->from()) {
        return d->mMessage->from()->asUnicodeString();
    } else {
        return QString();
    }
}

QString MessageParser::sender() const
{
    if (d->mMessage && d->mMessage->sender()) {
        return d->mMessage->sender()->asUnicodeString();
    } else {
        return QString();
    }
}
QString MessageParser::to() const
{
    if (d->mMessage && d->mMessage->to()) {
        return d->mMessage->to()->asUnicodeString();
    } else {
        return i18nc("displayed when a mail has unknown sender, receiver or date", "Unknown");
    }
}

QDateTime MessageParser::date() const
{
    if (d->mMessage && d->mMessage->date()) {
        return d->mMessage->date()->dateTime();
    } else {
        return QDateTime();
    }
}
