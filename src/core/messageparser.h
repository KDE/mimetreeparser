// SPDX-FileCopyrightText: 2016 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once
#include <QObject>
#include <QString>

#include <KMime/Message>
#include <QAbstractItemModel>

#include "mimetreeparser_core_export.h"
#include <memory>

class MessagePartPrivate;
class PartModel;
class AttachmentModel;

class MIMETREEPARSER_CORE_EXPORT MessageParser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(KMime::Message::Ptr message READ message WRITE setMessage NOTIFY htmlChanged)
    Q_PROPERTY(PartModel *parts READ parts NOTIFY htmlChanged)
    Q_PROPERTY(QAbstractItemModel *attachments READ attachments NOTIFY htmlChanged)
    Q_PROPERTY(QString structureAsString READ structureAsString NOTIFY htmlChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY htmlChanged)

    Q_PROPERTY(QString subject READ subject NOTIFY htmlChanged)
    Q_PROPERTY(QString from READ from NOTIFY htmlChanged)
    Q_PROPERTY(QString sender READ sender NOTIFY htmlChanged)
    Q_PROPERTY(QString to READ to NOTIFY htmlChanged)
    Q_PROPERTY(QString cc READ cc NOTIFY htmlChanged)
    Q_PROPERTY(QString bcc READ bcc NOTIFY htmlChanged)
    Q_PROPERTY(QDateTime date READ date NOTIFY htmlChanged)

public:
    explicit MessageParser(QObject *parent = Q_NULLPTR);
    ~MessageParser();

    KMime::Message::Ptr message() const;
    void setMessage(const KMime::Message::Ptr message);
    PartModel *parts() const;
    AttachmentModel *attachments() const;
    QString structureAsString() const;
    bool loaded() const;

    QString subject() const;
    QString from() const;
    QString sender() const;
    QString to() const;
    QString cc() const;
    QString bcc() const;
    QDateTime date() const;

Q_SIGNALS:
    void htmlChanged();

private:
    std::unique_ptr<MessagePartPrivate> d;
};
