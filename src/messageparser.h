// SPDX-FileCopyrightText: 2016 Christian Mollekopf <mollekopf@kolabsys.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once
#include <QObject>
#include <QString>

#include <KMime/Message>
#include <QAbstractItemModel>

#include <memory>

class MessagePartPrivate;

class MessageParser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(KMime::Message::Ptr message READ message WRITE setMessage NOTIFY htmlChanged)
    Q_PROPERTY(QAbstractItemModel *parts READ parts NOTIFY htmlChanged)
    Q_PROPERTY(QAbstractItemModel *attachments READ attachments NOTIFY htmlChanged)
    Q_PROPERTY(QString structureAsString READ structureAsString NOTIFY htmlChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY htmlChanged)

public:
    explicit MessageParser(QObject *parent = Q_NULLPTR);
    ~MessageParser();

    KMime::Message::Ptr message() const;
    void setMessage(const KMime::Message::Ptr message);
    QAbstractItemModel *parts() const;
    QAbstractItemModel *attachments() const;
    QString structureAsString() const;
    bool loaded() const;

Q_SIGNALS:
    void htmlChanged();

private:
    std::unique_ptr<MessagePartPrivate> d;
};
