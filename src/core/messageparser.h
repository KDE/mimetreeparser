// SPDX-FileCopyrightText: 2016 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once
#include <QObject>
#include <QString>

#include <KMime/Message>
#include <QAbstractItemModel>

#include "mimetreeparser_core_export.h"
#include "partmodel.h"
#include <memory>

class MessagePartPrivate;
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
    explicit MessageParser(QObject *parent = nullptr);
    ~MessageParser();

    [[nodiscard]] KMime::Message::Ptr message() const;
    void setMessage(const KMime::Message::Ptr message);
    [[nodiscard]] PartModel *parts() const;
    [[nodiscard]] AttachmentModel *attachments() const;
    [[nodiscard]] QString structureAsString() const;
    [[nodiscard]] bool loaded() const;

    [[nodiscard]] QString subject() const;
    [[nodiscard]] QString from() const;
    [[nodiscard]] QString sender() const;
    [[nodiscard]] QString to() const;
    [[nodiscard]] QString cc() const;
    [[nodiscard]] QString bcc() const;
    [[nodiscard]] QDateTime date() const;

Q_SIGNALS:
    void htmlChanged();

private:
    std::unique_ptr<MessagePartPrivate> d;
};
